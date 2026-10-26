***

# 🔑 PassKey HID — Аппаратный менеджер паролей на ESP32

**PassKey HID** — это персональный аппаратный менеджер паролей на базе микроконтроллера ESP32. Он хранит ваши пароли в зашифрованном виде (AES-256) и вводит их в компьютер как обычная USB-клавиатура. Устройство не требует установки драйверов и работает в любой операционной системе (Windows, macOS, Linux, Android). Репозиторий содержит исходники прошивки и файлы для печати.

![Устройство в сборе](Device.jpg)

## 🌟 Возможности
- **Эмуляция USB-клавиатуры:** Не требует драйверов, работает из коробки везде.
- **Аппаратное шифрование:** Все пароли шифруются алгоритмом AES-256 (режим CTR). Ключи генерируются рандомно и хранятся в защищенной памяти контроллера.
- **3 ячейки памяти:** 3 физические кнопки для мгновенного ввода трех разных паролей.
- **Встроенный веб-интерфейс:** Настройка паролей через удобный Captive Portal по Wi-Fi (с телефона или ноутбука).
- **Функция «Паника»:** Мгновенное физическое уничтожение всех паролей и ключей шифрования при угрозе.
- **Генератор паролей:** Встроенная кнопка в веб-интерфейсе для генерации стойких 16-символьных паролей.

---

## ⚠️ ВАЖНОЕ ПРАВИЛО ПЕРЕД ИСПОЛЬЗОВАНИЕМ
Так как устройство эмулирует USB-клавиатуру, оно отправляет скан-коды клавиш согласно стандартной английской раскладке (US).
**Перед тем как нажать кнопку для ввода пароля, ВСЕГДА переключайте язык ввода на компьютере на английский (EN).**
Если у вас будет включена русская раскладка (RU), пароль введется некорректно.

---

## 🛠 Сборка и установка (Прошивка)

### Подключение компонентов
Для сборки устройства вам понадобится плата ESP32-S3 Mini (встроенный RGB-светодиод), 2 тактовые кнопки.

| Компонент | Пин ESP32-S3 |
| :--- | :--- |
| Кнопка 2 | GPIO 6 |
| Кнопка 3 | GPIO 7 |

### Прошивка через PlatformIO (рекомендуется)
1. Установите [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html) (или плагин для VS Code).
2. Клонируйте данный репозиторий:
   ```bash
   git clone https://github.com/ВАШ_НИК/PassKey-HID.git
   cd PassKey-HID
   ```
3. В проекте уже настроен файл `platformio.ini`. Обратите внимание на важные параметры сборки (они отключают CDC и включают USB-HID):
   ```ini
   [env:esp32-s3]
   platform = espressif32
   board = esp32-s3-devkitc-1
   framework = arduino
   board_upload.flash_size = 4MB
   board_build.partitions = default.csv
   monitor_speed = 115200
   upload_speed = 921600
   lib_deps = gyverlibs/GyverButton @ ^3.8
   build_unflags = -D ARDUINO_USB_MODE=1 -D ARDUINO_USB_CDC_ON_BOOT=1
   build_flags = -D ARDUINO_USB_MODE=0 -D ARDUINO_USB_CDC_ON_BOOT=0
   ```
4. Подключите плату ESP32-S3 к компьютеру.
5. Нажмите кнопку **Upload** (Прошить) в PlatformIO.

---

## 📡 Индикация (RGB-светодиод)
Устройство общается с вами с помощью цветов:
- 🟣 **Фиолетовый** при включении — устройство загружается.
- 🟢 **Зеленый** — пароль успешно введен в поле.
- 🔵 **Синий** — активирован режим настройки (точка доступа Wi-Fi включена).
- 🟡 **Желтый** — пароли успешно сохранены через веб-интерфейс, готовится перезагрузка.
- 🔴 **Красный** — сработала функция «Паника» (полное стирание данных).
- ⚫ **Выключен** — устройство в режиме ожидания.

---

## ⚙️ Первичная настройка и смена паролей

По умолчанию ячейки памяти пусты. Чтобы записать пароли, нужно перевести устройство в режим настройки.

1. Подключите устройство к ПК или повербанку. Светодиод мигнет фиолетовым при старте.
2. **Нажмите и удерживайте любую одну из трех кнопок** в течение 3 секунд.
3. Светодиод загорится синим — устройство создало Wi-Fi сеть.
4. Возьмите телефон или ноутбук и найдите Wi-Fi сеть:
    - **SSID:** `MyPassKey`
    - **Пароль по умолчанию:** `12345678` *(если вы ранее задавали свой пароль для точки доступа, используйте его).*
5. После подключения подождите — автоматически всплывет окно авторизации (Captive Portal). Нажмите на него.
6. В открывшемся веб-интерфейсе вы увидите 4 поля:
    - **Пароль от этой Wi-Fi сети** — это пароль для защиты самой точки доступа устройства. Минимум 8 символов. Если не хотите менять, оставьте пустым.
    - **Кнопка 1, 2, 3** — сюда вписываются пароли, которые будут вводиться при нажатии на соответствующие кнопки.
7. Вы можете нажать кнопку **«Сгенерировать»**, чтобы устройство само создало стойкий 16-символьный пароль.
8. Нажмите **«Сохранить в память»**.
9. Светодиод загорится желтым, и устройство перезагрузится. Пароли зашифрованы (AES-256) и сохранены в энергонезависимую память.

*Примечание: Если вы оставите веб-интерфейс открытым и ничего не будете делать 5 минут, устройство автоматически перезагрузится для экономии заряда.*

---

## ⌨️ Ежедневное использование (Ввод пароля)

1. Подключите устройство к USB-порту компьютера.
2. Убедитесь, что **раскладка клавиатуры переключена на английскую (EN)**.
3. Поставьте курсор мыши в поле ввода пароля на сайте или в программе.
4. **Коротко нажмите** нужную кнопку (1, 2 или 3).
5. Светодиод на секунду загорится зеленым, и ваш пароль моментально напечатается в поле.

Если в ячейке пусто, устройство напечатает сообщение: `[ERROR: MEMORY IS EMPTY]`.

---

## 🚨 Функция «Паника» (Полное стирание)

Если вы боитесь, что устройство попало в чужие руки, или вы хотите вернуть его к заводским настройкам, вы можете мгновенно уничтожить все пароли и ключи шифрования.

1. **Одновременно нажмите и удерживайте все три кнопки**.
2. Удерживайте их около 3 секунд.
3. Светодиод загорится **ярко-красным** и будет гореть 2 секунды.
4. Устройство полностью очистит раздел с паролями и удалит криптографические ключи шифрования.
5. После этого произойдет перезагрузка.

⚠️ **ВНИМАНИЕ:** После активации «Паники» восстановить старые пароли будет **невозможно**, так как ключи шифрования были физически удалены из памяти. При следующем входе в настройки устройством будут сгенерированы новые ключи.

---

## 📝 Лицензия
Проект распространяется под лицензией **GNU General Public License v3.0 (GPL-3.0)**. Вы можете свободно использовать, модифицировать и распространять код, но любые производные работы должны распространяться под той же лицензией с открытым исходным кодом. Автор не несет ответственности за потерю данных или взлом паролей. Используйте на свой страх и риск.

***
***

# 🇬🇧 English Version

# 🔑 PassKey HID — Hardware Password Manager on ESP32

**PassKey HID** is a personal hardware password manager based on the ESP32 microcontroller. It stores your passwords in an encrypted form (AES-256) and types them into your computer as a standard USB keyboard. The device requires no drivers and works on any operating system (Windows, macOS, Linux, Android). Repository consists of firmware source and .step files for printing and modification.

![Device as a whole](Device.jpg)

## 🌟 Features
- **USB Keyboard Emulation:** Requires no drivers, works out of the box everywhere.
- **Hardware Encryption:** All passwords are encrypted using the AES-256 algorithm (CTR mode). Keys are generated randomly and stored in the controller's protected memory.
- **3 Memory Slots:** 3 physical buttons for instant input of three different passwords.
- **Built-in Web Interface:** Password configuration via a convenient Wi-Fi Captive Portal (via phone or laptop).
- **Panic Function:** Instant physical destruction of all passwords and encryption keys in case of a threat.
- **Password Generator:** Built-in button in the web interface to generate strong 16-character passwords.

---

## ⚠️ IMPORTANT RULE BEFORE USE
Since the device emulates a USB keyboard, it sends key scan codes according to the standard US layout.
**Before pressing the button to enter a password, ALWAYS switch the input language on your computer to English (EN).**
If you have a non-US layout (e.g., Russian) enabled, the password will be entered incorrectly.

---

## 🛠 Assembly and Installation (Flashing)

### Wiring Components
To assemble the device, you will need an ESP32-S3 Mini board (RGB LED built-in), 2 momentary push button.

| Component | ESP32-S3 Pin |
| :--- | :--- |
| Button 2 | GPIO 6 |
| Button 3 | GPIO 7 |
| Common Ground (GND) for buttons and LED | GPIO 4, GPIO 9 *(used as GND outputs in code)* |

### Flashing via PlatformIO (Recommended)
1. Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html) (or the VS Code plugin).
2. Clone this repository:
   ```bash
   git clone https://github.com/YOUR_USERNAME/PassKey-HID.git
   cd PassKey-HID
   ```
3. The `platformio.ini` file is already configured. Pay attention to the important build flags (they disable CDC and enable USB-HID):
   ```ini
   [env:esp32-s3]
   platform = espressif32
   board = esp32-s3-devkitc-1
   framework = arduino
   board_upload.flash_size = 4MB
   board_build.partitions = default.csv
   monitor_speed = 115200
   upload_speed = 921600
   lib_deps = gyverlibs/GyverButton @ ^3.8
   build_unflags = -D ARDUINO_USB_MODE=1 -D ARDUINO_USB_CDC_ON_BOOT=1
   build_flags = -D ARDUINO_USB_MODE=0 -D ARDUINO_USB_CDC_ON_BOOT=0
   ```
4. Connect the ESP32-S3 board to your computer.
5. Click the **Upload** button in PlatformIO.

---

## 📡 Indication (RGB LED)
The device communicates with you using colors:
- 🟣 **Purple** on boot — the device is booting.
- 🟢 **Green** — password successfully entered into the field.
- 🔵 **Blue** — configuration mode activated (Wi-Fi access point enabled).
- 🟡 **Yellow** — passwords successfully saved via the web interface, preparing to reboot.
- 🔴 **Red** — "Panic" function triggered (complete data wipe).
- ⚫ **Off** — device is in standby mode.

---

## ⚙️ Initial Setup and Changing Passwords

By default, the memory slots are empty. To save passwords, you need to put the device into configuration mode.

1. Plug the device into a PC or power bank. The LED will flash purple on startup.
2. **Press and hold any one of the three buttons** for 3 seconds.
3. The LED will light up blue — the device has created a Wi-Fi network.
4. Take your phone or laptop and find the Wi-Fi network:
    - **SSID:** `MyPassKey`
    - **Default password:** `12345678` *(if you previously set your own password for the access point, use it).*
5. After connecting, wait a moment — an authorization window (Captive Portal) will pop up automatically. Click on it.
6. In the opened web interface, you will see 4 fields:
    - **Password for this Wi-Fi network** — this is the password to protect the device's own access point. Minimum 8 characters. Leave blank if you don't want to change it.
    - **Button 1, 2, 3** — enter the passwords that will be typed when the corresponding button is pressed.
7. You can click the **"Generate"** button to have the device create a strong 16-character password for you.
8. Click **"Save to Memory"**.
9. The LED will light up yellow, and the device will reboot. The passwords are encrypted (AES-256) and saved to non-volatile memory.

*Note: If you leave the web interface open and do nothing for 5 minutes, the device will automatically reboot to save power.*

---

## ⌨️ Daily Usage (Password Input)

1. Plug the device into your computer's USB port.
2. Make sure your **keyboard layout is switched to English (EN)**.
3. Place your mouse cursor in the password input field on a website or in a program.
4. **Briefly press** the desired button (1, 2, or 3).
5. The LED will light up green for a second, and your password will be instantly typed into the field.

If a slot is empty, the device will print: `[ERROR: MEMORY IS EMPTY]`.

---

## 🚨 "Panic" Function (Complete Wipe)

If you are afraid the device has fallen into the wrong hands, or you want to return it to factory settings, you can instantly destroy all passwords and encryption keys.

1. **Simultaneously press and hold all three buttons**.
2. Hold them for about 3 seconds.
3. The LED will light up **bright red** and stay on for 2 seconds.
4. The device will completely clear the password partition and delete the cryptographic encryption keys.
5. After this, it will reboot.

⚠️ **WARNING:** After activating "Panic", it will be **impossible** to recover the old passwords, as the encryption keys have been physically deleted from memory. The next time you enter settings, the device will generate new keys.

---

## 📝 License
This project is licensed under the **GNU General Public License v3.0 (GPL-3.0)**. You are free to use, modify, and distribute the code, but any derivative works must be distributed under the same open-source license. The author is not responsible for data loss or password hacking. Use at your own risk.
