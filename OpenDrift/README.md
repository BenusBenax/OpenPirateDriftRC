# 🏴☠️ PirateDriftRC

> Форк [OpenDriftRC / OpenDrift](https://github.com) — гироскопического RC-дрифта —
> адаптированный для **дешёвой платы** с **дешёвым гироскопом MPU6050 (GY-521)**.

Проект — это контроллер стабилизации/дрифта для RC-дрифт-машины: берёт данные с
6-осевого инерциального сенсора, считает гироскопическую коррекцию руления и выдаёт
PWM на рулевое серво. Управляется и настраивается через веб-интерфейс по Wi-Fi.

PirateDriftRC поворачивает OpenDrift так, чтобы вместо дорогого дисплейного девайса
и встроенного QMI8658 использовать **обычный ESP32-C3 + модуль MPU6050 за копейки**.

---

## Зачем этот форк

Исходный OpenDrift рассчитан на платку с экраном, тачскрином и встроенным
инерциальным сенсором QMI8658. Это дорого и не всем доступно.

PirateDriftRC:
- работает **без дисплея, тачскрина и UI** (полностью headless);
- использует **MPU6050 (GY-521) по I²C** вместо встроенного QMI8658;
- целится в **ESP32-C3** — одну из самых дешёвых плат Espressif (RISC-V);
- настройка — через **веб-конфигуратор по Wi-Fi**, а не с экрана.

---

## Что вырезано / изменено из оригинала

| Изменение | Детали |
|-----------|--------|
| **Убран дисплей** | Весь UI/тачскрин (LovyanGFX, CST816S и пр.) изолирован `#if !defined(OPENDRIFT_BOARD_HEADLESS)` и не компилируется |
| **Заменён IMU** | Встроенный QMI8658 → внешний **MPU6050 (GY-521)** по I²C (`SDA=GPIO6`, `SCL=GPIO7`) |
| **Другая плата** | Целевая плата — `esp32-c3-devkitm-1` (RISC-V), flash режим `dio` (без bootloop) |
| **PWM-вход** | Радио читается как классический PWM (GPIO20 руление / GPIO21 газ), а не только CRSF |
| **Serial через USB** | На C3 Serial идёт через нативный USB-CDC, GPIO20/21 не заняты UART |
| **+ Веб-тест железа** | Добавлены кнопки **Test Servo / Test Motor** с защитным чекбоксом, тест бежит во FreeRTOS-задаче |

Блоки гиро-стабилизации (PirateDrift v1.0 response), настройка профилей, чёрный ящик
(blackbox), Wi-Fi-конфигуратор — сохранены и работают.

---

## Поддерживаемые сборки (PlatformIO)

- **`esp32_headless`** — PWM-радио + MPU6050 (основная, по умолчанию).
- **`esp32_headless_crsf`** — ExpressLRS/CRSF-приёмник + MPU6050.

## Библиотеки

Внешние зависимости (`platformio.ini` → `lib_deps`):

| Библиотека | Назначение |
|------------|------------|
| **`electroniccats/MPU6050 @ ^1.4.5`** | Драйвер дешёвого гироскопа GY-521 (MPU6050) |
| **`madhephaestus/ESP32Servo @ ^3.2.1`** | Управление рулевым серво (PWM/LEDC) |

Внутренние модули (`lib/`): `GyroController`, `IMU`, `RadioInput`, `Servo`,
`EscOutput`, `Settings`, `WebConfigurator`, `BlackboxLogger`, `WIFIManager`,
`CrsfInput`, `CrsfParameterDevice`, и т.д.

---

## Быстрый старт

```bash
# 1. Установите PlatformIO
pip install platformio

# 2. Соберите прошивку
pio run -e esp32_headless

# 3. Загрузите на ESP32-C3
pio run -e esp32_headless -t upload
```

После загрузки:
1. Подключитесь к Wi-Fi точке **`PirateDriftRC`** (пароль по умолчанию `piratedrift`).
2. Откройте веб-конфигуратор (обычно `http://192.168.4.1`).
3. Настройте серво/гироскоп, профили дрифта, а железо проверьте вкладкой
   **Hardware Test** (чекбокс “Unlock” → кнопки Test Servo / Test Motor).

---

## Сборка и железо (кратко)

- Плата: **ESP32-C3 devkitm-1** · прошивка на **4 MB** · flash `dio`.
- Гироскоп: **MPU6050** на цифрах 6/7.
- Серво на **GPIO2**, PWM-радио **GPIO20/21**, ESC-выход (тест) **GPIO1**.
- Серво/ESC питаются **внешне**, общий GND — обязательно.

Подробности — в [`docs/PINOUT.md`](docs/PINOUT.md).

---

## Лицензия / благодарности

Спасибо автору **OpenDriftRC** за отличный гироскопический дрифт и его открытый код.
Этот репозиторий — экспериментальный форк под дешёвое железо.

---

# 🏴☠️ PirateDriftRC (English)
> A fork of the gyroscopic RC drift controller [OpenDriftRC / OpenDrift](https://github.com),
> re-targeted to run on a **cheap board with a cheap MPU6050 (GY-521) gyro**.

Instead of a fancy display unit with a built-in QMI8658 sensor, PirateDriftRC runs on a
humble **ESP32-C3 + MPU6050** — a few-dollar combination — while keeping the drift
stabilization core intact.

## Why this fork

The upstream OpenDrift targets a board with a screen, touch display and an integrated
QMI8658 inertial sensor. That is expensive. PirateDriftRC:

- runs **fully headless** (no display / touch / UI);
- reads a **MPU6050 (GY-521) over I²C** instead of the built-in QMI8658;
- targets the **ESP32-C3** — one of the cheapest Espressif boards (RISC-V);
- is configured through the **Wi-Fi web configurator**, not the screen.

## What was removed / changed

| Change | Details |
|--------|---------|
| **Removed display** | All UI/touch logic (LovyanGFX, CST816S…) is isolated behind `#if !defined(OPENDRIFT_BOARD_HEADLESS)` and not compiled |
| **Replaced IMU** | Built-in QMI8658 → external **MPU6050 (GY-521)** over I²C (`SDA=GPIO6`, `SCL=GPIO7`) |
| **Different board** | Target is `esp32-c3-devkitm-1` (RISC-V), flash mode `dio` (no bootloop) |
| **PWM input** | Radio is read as classic PWM (GPIO20 steering / GPIO21 throttle), not only CRSF |
| **Serial over USB** | On C3 Serial uses the native USB-CDC, so GPIO20/21 stay free for radio |
| **+ Web hardware test** | Added **Test Servo / Test Motor** buttons behind a safety checkbox, running in a FreeRTOS task |

The gyro stabilization (PirateDrift v1.0 response), driving profiles, blackbox logger
and the WiFi configurator are all kept and working.

## Build targets (PlatformIO)

- **`esp32_headless`** — PWM radio + MPU6050 (default).
- **`esp32_headless_crsf`** — ExpressLRS/CRSF receiver + MPU6050.

Full pinout: [`docs/PINOUT.md`](docs/PINOUT.md).

## Libraries

External dependencies (`platformio.ini` → `lib_deps`):

| Library | Purpose |
|---------|---------|
| **`electroniccats/MPU6050 @ ^1.4.5`** | Driver for the cheap GY-521 (MPU6050) gyro |
| **`madhephaestus/ESP32Servo @ ^3.2.1`** | Steering servo control (PWM/LEDC) |

Internal modules (`lib/`): `GyroController`, `IMU`, `RadioInput`, `Servo`,
`EscOutput`, `Settings`, `WebConfigurator`, `BlackboxLogger`, `WIFIManager`,
`CrsfInput`, `CrsfParameterDevice`, etc.

## Quick start

```bash
pip install platformio
pio run -e esp32_headless
pio run -e esp32_headless -t upload
```

After flashing:
1. Join the **`PirateDriftRC`** Wi-Fi AP (default password `piratedrift`).
2. Open the web configurator (usually `http://192.168.4.1`).
3. Tune the servo/gyro and driving profiles; use the **Hardware Test** card
   (tick “Unlock”, then **Test Servo / Test Motor**) to verify wiring.

> ⚠️ Servo/ESC are powered **externally**; a shared ground is required.
> See `docs/PINOUT.md` for pin details.

## License / credits

Credits to the **OpenDriftRC** author for an excellent gyroscopic drift controller and
its open source. This repository is an experimental fork for cheap hardware.

---

🏴☠️ **Йо-хо-хо!** И бутылка рома! 🏴☠️
**ARRR!** Пиратский дрифт построен на копеечном железе, приятель. ⛵
Полная распиновка в [`docs/PINOUT.md`](docs/PINOUT.md).