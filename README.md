# ESP32-S3 ZJY169S0800TG01 SPI Display Driver

This repository contains the embedded software, hardware documentation, and architectural specifications for driving a **Zhongjingyuan ZJY169S0800TG01** 1.69-inch IPS TFT display using the **Waveshare ESP32-S3-Zero** development board over 4-wire SPI with ESP-IDF.

---

## 📚 Project Documentation

Detailed hardware documentation, pinouts, photos, and controller details are organized in the [`doc/`](doc/) directory:

- 📟 **Development Board Documentation**: [`doc/devboard/DEVBOARD.md`](doc/devboard/DEVBOARD.md)
  - Waveshare ESP32-S3-Zero pinout, dimensions, strapping pins, power configuration, and schematics.
  - Official documentation: [Waveshare ESP32-S3-Zero Wiki](https://docs.waveshare.com/ESP32-S3-Zero?variant=ESP32-S3-Zero).
- 🖥️ **Display Module Documentation**: [`doc/display/DISPLAY.md`](doc/display/DISPLAY.md)
  - Zhongjingyuan ZJY169S0800TG01 physical connector pinout, ST7789V2 controller addressing, coordinate offsets (`240×280`), electrical caveats, and SPI mode details.
- 🤖 **Agentic & AI Guidelines**: [`GEMINI.md`](GEMINI.md) / [`AGENTS.md`](AGENTS.md)
  - Instructions and rules for Antigravity, GitHub Copilot, and other agentic pair programmers.
- 📐 **Specifications Directory**: [`spec/`](spec/)
  - Numbered design specs (`spec/XXX-<topic>.md`) for agentic feature planning and implementation.

---

## 🛠️ Hardware Overview

### 1. Development Board: Waveshare ESP32-S3-Zero

* **Microcontroller**: Espressif ESP32-S3FH4R2 (Dual-core Xtensa® LX7 up to 240 MHz)
* **Memory**: 512 KB SRAM, 4 MB Quad-SPI Flash, 2 MB Quad-SPI PSRAM
* **Connectivity**: 2.4 GHz Wi-Fi (802.11 b/g/n), Bluetooth® 5 (LE), onboard ceramic antenna
* **USB**: Native USB Type-C (CDC/JTAG console and programming)
* **Peripherals**: User buttons (`BOOT` / `RESET`), addressable WS2812 RGB LED on `GPIO 21`

### 2. Display Module: Zhongjingyuan ZJY169S0800TG01

| Property | Specification |
|---|---|
| **Screen** | 1.69-inch IPS TFT (Rounded corners, $R \approx 3.5\text{ mm} / \approx 30\text{ px}$) |
| **Resolution** | 240 × 280 pixels |
| **Controller** | Sitronix ST7789V2 |
| **Interface** | 4-wire SPI (Clock, MOSI, D/C, CS) + Reset & Backlight |
| **Supply Voltage** | **3.3 V** (Strictly avoid 5 V) |
| **Color** | RGB, 16-bit RGB565 supported (65,536 colors) |
| **Backlight** | White LEDs, controllable (PWM via LEDC) |
| **Touch** | None |
| **Connector** | 8-pin, 2.54 mm pitch |

---

## 🔌 Wiring & Pin Connections

Recommended connection mapping between the Waveshare ESP32-S3-Zero and the display module:

| Display Pin | Silk Label | ESP32-S3-Zero GPIO | Signal Description |
|:---:|:---:|:---:|---|
| **1** | `GND` | `GND` | Common Ground |
| **2** | `VCC` | `3V3` | 3.3V Power Supply |
| **3** | `SCL` | `GPIO 4` | SPI Clock (SCLK) |
| **4** | `SDA` | `GPIO 6` | SPI MOSI (Master Out Slave In) |
| **5** | `RES` | `GPIO 5` | Display Reset (Active Low) |
| **6** | `DC`  | `GPIO 7` | Data / Command Select |
| **7** | `CS`  | `GPIO 8` | Chip Select (Active Low) |
| **8** | `BLK` | `GPIO 9` | Backlight Enable / PWM Control |

> ⚠️ **Warning**: Display power and logic levels must be **3.3 V**. Do not connect the display to the 5V rail. Backlight should be driven with care; do not sink/source excessive current directly from ESP32 GPIOs.

---

## 🚀 Getting Started

### Prerequisites

- [Espressif ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/) (v5.0 or later recommended)
- CMake and Ninja build tools
- USB Type-C cable

### Build & Flash

1. **Set target**:
   ```bash
   idf.py set-target esp32s3
   ```
2. **Build the project**:
   ```bash
   idf.py build
   ```
3. **Flash to board and monitor output**:
   ```bash
   idf.py -p <PORT> flash monitor
   ```
   *(Replace `<PORT>` with your serial device, e.g. `COM3` on Windows or `/dev/ttyACM0` on Linux).*

---

## 📁 Repository Structure

```text
├── CMakeLists.txt          # Root CMake project configuration
├── README.md               # Project overview and hardware links
├── GEMINI.md               # Antigravity agent guidelines & constraints
├── AGENTS.md               # Copilot/agentic guidelines mirror
├── doc/
│   ├── devboard/           # ESP32-S3-Zero documentation & photos
│   │   ├── DEVBOARD.md
│   │   ├── board_dimentions.webp
│   │   ├── board_foto.webp
│   │   ├── board_pinout.webp
│   │   └── board_resources.webp
│   └── display/            # ZJY169S0800TG01 display documentation & photos
│       ├── DISPLAY.md
│       ├── display_front.jpg
│       └── display_back.jpg
├── main/
│   ├── CMakeLists.txt      # Main component CMake build script
│   └── main.c              # Application entry point (app_main)
└── spec/                   # Agentic development specifications
    └── README.md
```

