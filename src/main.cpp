#include <Arduino.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <GyverButton.h>
#include "mbedtls/aes.h"
#include "nvs_flash.h"

// ==========================================
// НАСТРОЙКИ ПИНОВ И ЖЕЛЕЗА
// ==========================================
const int BTN_PINS[3] = {0, 6, 7};
const int GND_PINS[2] = {4, 9};
const int RGB_PIN = 21;

GButton btn1(BTN_PINS[0]);
GButton btn2(BTN_PINS[1]);
GButton btn3(BTN_PINS[2]);

USBHIDKeyboard Keyboard;
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;
Preferences prefs;

bool isConfigMode = false;
unsigned long configStartTime = 0;

// ==========================================
// УПРАВЛЕНИЕ RGB
// ==========================================
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  neopixelWrite(RGB_PIN, r, g, b);
}

// ==========================================
// КРИПТОГРАФИЯ (AES-256 CTR)
// ==========================================
uint8_t aesKey[32];
uint8_t aesIV[16];

void initCrypto() {
  prefs.begin("crp", false); // Короткое имя пространства имен
  if (prefs.getBytesLength("key") != 32) {
    esp_fill_random(aesKey, 32);
    esp_fill_random(aesIV, 16);
    prefs.putBytes("key", aesKey, 32);
    prefs.putBytes("iv", aesIV, 16);
  } else {
    prefs.getBytes("key", aesKey, 32);
    prefs.getBytes("iv", aesIV, 16);
  }
  prefs.end();
}

void savePassword(int index, String plain) {
  if (plain.length() == 0) return;

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, aesKey, 256);

  unsigned char iv_copy[16];
  memcpy(iv_copy, aesIV, 16);

  size_t nc_off = 0;
  size_t len = plain.length();
  unsigned char stream_block[16];
  unsigned char* encrypted = new unsigned char[len];

  mbedtls_aes_crypt_ctr(&aes, len, &nc_off, iv_copy, stream_block, (const unsigned char*)plain.c_str(), encrypted);
  mbedtls_aes_free(&aes);

  prefs.begin("pwd", false);
  String key = "p" + String(index);
  prefs.putBytes(key.c_str(), encrypted, len);
  prefs.end();

  delete[] encrypted;
}

String loadPassword(int index) {
  prefs.begin("pwd", true);
  String key = "p" + String(index);
  size_t len = prefs.getBytesLength(key.c_str());
  if (len == 0) {
    prefs.end();
    return ""; // Если в памяти ничего нет
  }

  unsigned char* encrypted = new unsigned char[len];
  prefs.getBytes(key.c_str(), encrypted, len);
  prefs.end();

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, aesKey, 256);

  unsigned char iv_copy[16];
  memcpy(iv_copy, aesIV, 16);

  size_t nc_off = 0;
  unsigned char stream_block[16];
  unsigned char* decrypted = new unsigned char[len + 1];

  mbedtls_aes_crypt_ctr(&aes, len, &nc_off, iv_copy, stream_block, encrypted, decrypted);
  mbedtls_aes_free(&aes);

  decrypted[len] = '\0';
  String result = String((char*)decrypted);

  delete[] encrypted;
  delete[] decrypted;
  return result;
}

// ==========================================
// ВЕБ-СЕРВЕР И CAPTIVE PORTAL
// ==========================================
void handleRoot() {
  String html = R"=====(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Менеджер паролей</title>
<style>
  body { font-family: 'Segoe UI', Tahoma, sans-serif; background: #1a1a2e; color: #fff; padding: 20px; display: flex; justify-content: center; margin: 0;}
  .card { background: #16213e; padding: 20px; border-radius: 12px; box-shadow: 0 10px 25px rgba(0,0,0,0.8); width: 100%; max-width: 420px; }
  h2 { text-align: center; color: #e94560; margin-bottom: 20px; text-transform: uppercase; letter-spacing: 1px;}
  .group { margin-bottom: 20px; background: #0f3460; padding: 15px; border-radius: 8px; }
  .wifi-group { background: #e9456022; border: 1px solid #e94560; }
  label { display: block; margin-bottom: 8px; font-weight: bold; font-size: 14px; color: #ccc;}
  input { width: 100%; padding: 12px; margin-bottom: 12px; border: none; border-radius: 5px; box-sizing: border-box; background: #222; color: #0f0; font-family: monospace; font-size: 16px; outline: none;}
  button { width: 100%; padding: 12px; border: none; border-radius: 5px; cursor: pointer; font-weight: bold; transition: 0.2s; }
  .btn-gen { background: #e94560; color: #fff; }
  .btn-gen:hover { background: #d83a50; }
  .btn-save { background: #4caf50; color: #fff; font-size: 18px; padding: 16px; margin-top: 10px; text-transform: uppercase;}
  .btn-save:hover { background: #45a049; }
  .note { font-size: 12px; color: #888; text-align: center; margin-top: 15px; line-height: 1.5; }
</style>
<script>
  function g(id) {
    const c = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+~=";
    let p = ""; for(let i=0; i<16; i++) p += c[Math.floor(Math.random()*c.length)];
    document.getElementById(id).value = p;
  }
</script>
</head><body>
<div class="card">
  <h2>Настройки ключа</h2>
  <form action="/save" method="POST">

    <div class='group wifi-group'>
      <label>🔒 ПАРОЛЬ ОТ ЭТОЙ WI-FI СЕТИ</label>
      <input type='text' name='p0' id='p0' placeholder='Оставьте пустым для сохранения текущего'>
      <div style="font-size:11px; color:#aaa; margin-top:-5px;">Минимум 8 символов.</div>
    </div>
)=====";

  for (int i = 1; i <= 3; i++) {
    html += "<div class='group'><label>КНОПКА " + String(i) + "</label>";
    html += "<input type='text' name='p" + String(i) + "' id='p" + String(i) + "' placeholder='Введите новый или оставьте пустым'>";
    html += "<button type='button' class='btn-gen' onclick=\"g('p" + String(i) + "')\">Сгенерировать</button></div>";
  }

  html += R"=====(
    <button type="submit" class="btn-save">Сохранить в память</button>
  </form>
  <div class="note">⚠️ Текущие пароли скрыты.</div>
</div></body></html>
  )=====";
  server.send(200, "text/html", html);
}

void handleSave() {
  String p0 = server.arg("p0");
  String p1 = server.arg("p1");
  String p2 = server.arg("p2");
  String p3 = server.arg("p3");

  if (p0.length() >= 8) savePassword(0, p0);
  if (p1.length() > 0) savePassword(1, p1);
  if (p2.length() > 0) savePassword(2, p2);
  if (p3.length() > 0) savePassword(3, p3);

  setRGB(100, 80, 0);

  String html = "<!DOCTYPE html><html><body style='background:#1a1a2e; color:#0f0; font-family:sans-serif; text-align:center; padding-top:50px;'>";
  html += "<h2>Успешно сохранено!</h2><p>Устройство перезагружается...</p></body></html>";

  server.send(200, "text/html", html);
  delay(1500);
  setRGB(0, 0, 0);
  ESP.restart();
}

void handleNotFound() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
  server.send(302, "text/plain", "");
}

// ==========================================
// СТАРТ УСТРОЙСТВА
// ==========================================
void setup() {
  // На всякий случай оставляем защиту от битой флешки
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }

  for (int i = 0; i < 2; i++) {
    pinMode(GND_PINS[i], OUTPUT);
    digitalWrite(GND_PINS[i], LOW);
  }

  setRGB(50, 0, 100);
  delay(400);
  setRGB(0, 0, 0);

  btn1.setTimeout(3000);
  btn2.setTimeout(3000);
  btn3.setTimeout(3000);

  initCrypto();
  Keyboard.begin();
  USB.productName("PassKey HID"); // Заставляем ПК увидеть новое устройство
  USB.manufacturerName("WireRod");
  USB.begin();
}
// ==========================================
// ГЛАВНЫЙ ЦИКЛ
// ==========================================
void loop() {
  if (isConfigMode) {
    dnsServer.processNextRequest();
    server.handleClient();
    if (millis() - configStartTime > 300000) ESP.restart();
    return;
  }

  btn1.tick();
  btn2.tick();
  btn3.tick();

  bool c1 = btn1.isClick();
  bool c2 = btn2.isClick();
  bool c3 = btn3.isClick();

  // --- КОРОТКИЕ НАЖАТИЯ (Ввод пароля) ---
  if (c1 || c2 || c3) {
    setRGB(0, 100, 0);

    String p = "";
    if (c1) p = loadPassword(1);
    else if (c2) p = loadPassword(2);
    else if (c3) p = loadPassword(3);

    if (p == "") {
      Keyboard.print("[ERROR: MEMORY IS EMPTY]");
    } else {
      Keyboard.print(p.c_str());
    }

    delay(50);
    setRGB(0, 0, 0);
  }

  bool h1 = btn1.isHold();
  bool h2 = btn2.isHold();
  bool h3 = btn3.isHold();

  // --- ДОЛГОЕ НАЖАТИЕ ---
  if (h1 || h2 || h3) {

    // ПРОВЕРКА НА ПАНИК-КНОПКУ (Зажаты ли ВСЕ ТРИ кнопки физически?)
    if (btn1.state() && btn2.state() && btn3.state()) {

      setRGB(255, 0, 0); // Врубаем КРАСНЫЙ

      // Полностью форматируем раздел с паролями
      prefs.begin("pwd", false);
      prefs.clear();
      prefs.end();

      // И удаляем крипто-ключи (при следующей загрузке сгенерируются новые)
      prefs.begin("crp", false);
      prefs.clear();
      prefs.end();

      delay(2000); // Горим красным 2 секунды, чтобы юзер понял, что всё удалено
      setRGB(0, 0, 0);
      ESP.restart(); // Перезагружаемся в чистом виде

    } else {

      // ОБЫЧНЫЙ РЕЖИМ WI-FI (Зажата одна кнопка)
      setRGB(0, 50, 150);

      String apPass = loadPassword(0);
      if (apPass.length() < 8) apPass = "12345678";

      WiFi.softAP("MyPassKey", apPass.c_str());
      dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

      server.on("/", handleRoot);
      server.on("/save", HTTP_POST, handleSave);
      server.onNotFound(handleNotFound);
      server.begin();

      isConfigMode = true;
      configStartTime = millis();
    }
  }
}
