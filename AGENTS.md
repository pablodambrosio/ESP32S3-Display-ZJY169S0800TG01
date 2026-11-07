# Agent Instructions & Project Guidelines (Antigravity & Copilot)

> **Note for AI Agents:** This file and [`GEMINI.md`](GEMINI.md) provide the rules and hardware constraints for this repository. They are kept in sync for cross-agent compatibility (Antigravity, GitHub Copilot, Cursor).

---

## 1. Project Overview

This project is an **ESP-IDF (C / FreeRTOS)** embedded application targeting the **Waveshare ESP32-S3-Zero** development board to drive a **Zhongjingyuan ZJY169S0800TG01 1.69" IPS TFT display** via 4-wire SPI.

* **Target MCU**: ESP32-S3 (ESP32-S3FH4R2, dual-core Xtensa LX7 @ 240 MHz)
* **Target Display**: ZJY169S0800TG01 (1.69", 240×280 pixels, ST7789V2 controller)
* **Framework**: Espressif ESP-IDF (v5.x recommended)
* **Build System**: CMake / Ninja (`idf.py`)

---

## 2. Hardware Documentation & Reference Links

Hardware details, pinouts, and electrical caveats are located in `doc/`:

* **Development Board Specs**: [`doc/devboard/DEVBOARD.md`](doc/devboard/DEVBOARD.md)
  * Waveshare Wiki: https://docs.waveshare.com/ESP32-S3-Zero?variant=ESP32-S3-Zero
  * Embedded images: `doc/devboard/*.webp`
* **Display Module Specs**: [`doc/display/DISPLAY.md`](doc/display/DISPLAY.md)
  * Screen specs, connector pinout, coordinate offsets, and backlight caveats
  * Embedded images: `doc/display/*.jpg`

---

## 3. Hardware Pin Assignments & Constraints

### Pin Mapping (ESP32-S3-Zero to ZJY169S0800TG01)

| Signal | Display Pin | ESP32-S3-Zero GPIO | Notes / Peripherals |
|---|:---:|:---:|---|
| **GND** | 1 | `GND` | Common ground rail |
| **VCC** | 2 | `3V3` | **Strictly 3.3 V** (never 5 V) |
| **SCL** | 3 | `GPIO 4` | SPI2 (FSPI) Clock (SCLK) |
| **SDA** | 4 | `GPIO 6` | SPI2 (FSPI) MOSI (Master-Out Slave-In) |
| **RES** | 5 | `GPIO 5` | Hardware reset (active low) |
| **DC**  | 6 | `GPIO 7` | Data / Command selector (0=Cmd, 1=Data) |
| **CS**  | 7 | `GPIO 8` | Chip select (active low) |
| **BLK** | 8 | `GPIO 9` | Backlight control (Binary GPIO ON/OFF default; optional LEDC PWM per Spec 003) |

### Critical Hardware Rules:
1. **Never use 5V for display power or logic**: The ST7789V2 is a 3.3V device.
2. **Backlight (`BLK`) Safety**: The manufacturer datasheet specifies `BLK` strictly as digital ON/OFF (High = ON, Low = OFF). Default to binary GPIO output (`GPIO 9`), verify logic-level base/gate current (< 2 mA into `U2`), and only enable LEDC PWM per [`spec/003-backlight-pwm-dimming.md`](spec/003-backlight-pwm-dimming.md). Never source raw LED backlight current directly from an ESP32 GPIO.
3. **Reserved GPIOs**:
   - `GPIO 33` to `GPIO 37` are bonded to internal Flash/PSRAM. **Do not use.**
   - `GPIO 21` is hardwired to the onboard WS2812 RGB LED.
   - `GPIO 0` is the `BOOT` strapping pin. Keep floating/high during boot.
   - `GPIO 19` & `GPIO 20` are reserved for native USB (D- / D+).

---

## 4. Display Controller (ST7789V2) Architecture

1. **Resolution, Memory Gap & Rounded Corners**:
   - Physical resolution is **240 × 280**, but the ST7789V2 controller GRAM is **240 × 320**.
   - In portrait mode (0°), set row offset **Y = 20**, column offset **X = 0**.
   - When rotating 90°, 180°, or 270°, recalculate gap offsets accordingly (see [`doc/display/DISPLAY.md`](doc/display/DISPLAY.md)).
   - The physical panel has **rounded corners with a radius of ~3.5 mm (~30 pixels)**. Inset corner UI elements or use $R = 30\text{ px}$ rounded borders so content is not clipped by the bezel.
2. **Color Mode**:
   - Use RGB565 (16-bit color, `COLMOD = 0x55`).
   - IPS displays generally require display inversion enabled (`INVON = 0x21` / `esp_lcd_panel_invert_color(..., true)`).
3. **ESP-IDF Driver API**:
   - Use the official `esp_lcd` driver (`esp_lcd_new_panel_io_spi` and `esp_lcd_new_panel_st7789`).
   - Enable DMA (`SPI_DMA_CH_AUTO`).
   - Ensure draw buffers sent to SPI DMA are allocated in **DMA-capable internal SRAM** (`MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL`).

---

## 5. Specification-Driven Workflow (`spec/`)

Before introducing substantial features, libraries, or architectural changes:
1. Check or create a numbered design specification in `spec/` (e.g. `spec/001-display-driver.md`).
2. Adhere to the established interfaces, data structures, and pinouts.
3. Keep specifications and documentation updated whenever pinout or software architecture evolves.

---

## 6. Coding & Build Standards

* **C Standards**: C99 / C11, clean header declarations (`#pragma once` or include guards).
* **Logging**: Use ESP-IDF logging macros (`ESP_LOGI`, `ESP_LOGW`, `ESP_LOGE`) with a defined `TAG`.
* **Error Handling**: Check `esp_err_t` using `ESP_ERROR_CHECK()` or graceful error returns.
* **Build Command**:
  ```bash
  idf.py build
  ```
* **Flash & Serial Monitor**:
  ```bash
  idf.py -p <PORT> flash monitor
  ```

