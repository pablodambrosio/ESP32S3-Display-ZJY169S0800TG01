# Specification 001: Board & Display Bring-up

## 1. Objective & Scope

Establish initial hardware initialization, serial diagnostics, and full display verification for the **Waveshare ESP32-S3-Zero** connected to the **Zhongjingyuan ZJY169S0800TG01** (1.69" 240×280 ST7789V2 IPS TFT) over 4-wire SPI.

### Deliverables
1. **System & Diagnostic Initialization**: Clock setup (240 MHz), native USB Serial/JTAG console logging, chip info print.
2. **Backlight Driver (`bsp_backlight`)**: Safe binary digital GPIO control (High = ON, Low = OFF) adhering strictly to the manufacturer specification, starting OFF during early initialization until the display GRAM is cleared.
3. **Display Driver (`bsp_display`)**: ESP-IDF `esp_lcd` integration with ST7789V2, SPI DMA, correct 240×280 offset geometry, and IPS color inversion.
4. **Bring-up Test Suite**: Visual self-test displaying a color bar sequence (Red, Green, Blue, White, Black) followed by a 1-pixel outer border to verify edge alignment.

> **Note on PWM Dimming**: Because the manufacturer datasheet only specifies digital ON/OFF for `BLK` and does not characterize the `U2` transistor switching circuit for PWM, PWM brightness control is deferred to a separate specification ([`spec/003-backlight-pwm-dimming.md`](003-backlight-pwm-dimming.md)) pending electrical verification.

---

## 2. Hardware Pinout & Electrical Constraints

| Display Signal | Display Pin | ESP32-S3-Zero Pin | Peripheral Function | Electrical Specification |
|---|:---:|:---:|---|---|
| **GND** | 1 | `GND` | System Ground | Common 0 V reference |
| **VCC** | 2 | `3V3` | 3.3 V Power Rail | **Strictly 3.3 V** (never 5 V) |
| **SCL** | 3 | `GPIO 4` | SPI2 FSPI SCLK | Max 40 MHz clock |
| **SDA** | 4 | `GPIO 6` | SPI2 FSPI MOSI | 1-bit serial data out |
| **RES** | 5 | `GPIO 5` | GPIO (Output) | Active-low hardware reset |
| **DC**  | 6 | `GPIO 7` | GPIO (Output) | Low = Command, High = Data |
| **CS**  | 7 | `GPIO 8` | SPI2 FSPI CS0 | Active-low hardware/software CS |
| **BLK** | 8 | `GPIO 9` | GPIO (Output) | Digital logic: High (3.3 V) = ON, Low (0 V) = OFF |

### Constraints:
- Display power and logic levels must never exceed 3.3 V.
- **Backlight (`BLK`) Safety**: Initialize `GPIO 9` as a standard digital output driven LOW (OFF) at boot. Verify with a multimeter during initial bring-up that `BLK` draws only logic-level base/gate current (< 1–2 mA) when driven HIGH, confirming `U2` switches the LED current from `VCC`.
- Avoid touching `GPIO 33–37` (internal Flash/PSRAM) and `GPIO 21` (onboard WS2812).

---

## 3. ST7789V2 Geometry & Memory Configuration

- **Controller Internal GRAM**: 240 columns × 320 rows.
- **Physical Visible Area**: 240 columns × 280 rows.
- **Offsets**:
  - **Portrait (0°)**: Column offset $X = 0$, Row offset $Y = 20$.
  - **Landscape (90°)**: Column offset $X = 20$, Row offset $Y = 0$.
- **Color Format**: RGB565 (16-bit, Big-Endian / MSB first).
- **Color Inversion**: Must be enabled (`esp_lcd_panel_invert_color(..., true)`) for accurate IPS color representation.

---

## 4. Software Architecture & API Design

### 4.1 Component Modularization

```text
main/
├── CMakeLists.txt
├── main.c                  # Bring-up test runner
└── bsp/
    ├── bsp_board.h / .c     # Board init, clock tree, system telemetry
    ├── bsp_backlight.h / .c # Binary GPIO backlight control (ON/OFF)
    └── bsp_display.h / .c   # esp_lcd ST7789V2 panel and SPI bus management
```

### 4.2 C API Definitions

#### `bsp_backlight.h`
```c
#pragma once
#include <esp_err.h>
#include <stdbool.h>

esp_err_t bsp_backlight_init(void);          // Initializes GPIO 9 as output, defaults to OFF (0)
esp_err_t bsp_backlight_set(bool enable);    // true = HIGH (ON), false = LOW (OFF)
esp_err_t bsp_backlight_on(void);            // Convenience wrapper for bsp_backlight_set(true)
esp_err_t bsp_backlight_off(void);           // Convenience wrapper for bsp_backlight_set(false)
```

#### `bsp_display.h`
```c
#pragma once
#include <esp_err.h>
#include <esp_lcd_types.h>

#define BSP_LCD_H_RES  240
#define BSP_LCD_V_RES  280

esp_err_t bsp_display_init(esp_lcd_panel_handle_t *out_panel, esp_lcd_panel_io_handle_t *out_io);
esp_err_t bsp_display_fill_color(uint16_t color);
esp_err_t bsp_display_draw_test_pattern(void);
```

### 4.3 Buffer Allocation
- Test buffers and bitmap transmission buffers must be allocated in **internal DMA-capable SRAM**:
  ```c
  uint16_t *buf = heap_caps_malloc(BSP_LCD_H_RES * 20 * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  ```

---

## 5. Implementation Phases

1. **Phase 1: Project Setup & Board Diagnostics**:
   - Configure `sdkconfig` for native USB JTAG/CDC console, 240 MHz CPU frequency, and Flash/PSRAM detection.
   - Implement `bsp_board_init()` to print chip revision, flash size, and available heap.
2. **Phase 2: Binary Backlight Driver & Electrical Check**:
   - Configure `GPIO 9` as `GPIO_MODE_OUTPUT` with pull-down/initial state `0` (backlight OFF).
   - Measure/verify logic current on `GPIO 9` when toggled to `1` (backlight ON).
3. **Phase 3: SPI Bus & Panel Initialization**:
   - Initialize SPI2 host with `spi_bus_initialize()`, DMA auto channel.
   - Configure `esp_lcd_new_panel_io_spi()` at 20 MHz.
   - Configure `esp_lcd_new_panel_st7789()` with reset on `GPIO 5`.
   - Apply gap: `esp_lcd_panel_set_gap(panel, 0, 20)`.
   - Apply color inversion: `esp_lcd_panel_invert_color(panel, true)`.
   - Clear GRAM to black (`0x0000`) before turning display and backlight ON to avoid showing random power-on GRAM noise.
   - Turn display on (`esp_lcd_panel_disp_on_off(panel, true)`) and enable backlight (`bsp_backlight_on()`).
4. **Phase 4: Bring-up Visual Tests**:
   - Fill screen: Red (`0xF800`), Green (`0x07E0`), Blue (`0x001F`), White (`0xFFFF`), Black (`0x0000`).
   - Draw 1-pixel boundary box around $(0, 0)$ to $(239, 279)$ to confirm no edge clipping.
   - Ramp SPI bus to 40 MHz and verify signal stability.

---

## 6. Acceptance & Verification Criteria

- [x] **Serial Console**: ESP-IDF boot logs appear cleanly over native USB CDC.
- [x] **Backlight Electrical Safety**: `GPIO 9` cleanly toggles the backlight ON and OFF with logic-level current (< 2 mA), and remains OFF during boot until GRAM is initialized.
- [x] **Color Accuracy**: Red is rendered as red (not blue/cyan), verifying correct endianness and inversion.
- [x] **Boundary Verification**: All 4 edges of the 1-pixel perimeter line are visible, confirming row offset `Y = 20` and column offset `X = 0`.
- [x] **DMA Transfer**: SPI DMA writes execute without timeouts or heap corruption.
