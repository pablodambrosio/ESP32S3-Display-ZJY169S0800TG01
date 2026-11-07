# Specification 003: Backlight PWM Dimming (Optional Extension)

## 1. Objective & Scope

Extend the binary `bsp_backlight` driver introduced in [`spec/001-board-display-bringup.md`](001-board-display-bringup.md) with optional **LEDC Pulse Width Modulation (PWM)** brightness control and hardware-assisted fade transitions.

Because the Zhongjingyuan manufacturer documentation only specifies the `BLK` pin as a **digital ON/OFF input** (`High = ON, Low = OFF`) and does not characterize the onboard `U2` transistor circuit for high-frequency switching, this feature is isolated into its own specification with mandatory hardware verification prerequisites and a compile-time safety toggle.

---

## 2. Hardware Prerequisites & Electrical Verification

Before enabling PWM on `GPIO 9` (`BLK`), the following electrical checks must be completed and recorded during Spec 001 bring-up:

1. **Gate/Base Current Verification**:
   - With `VCC = 3.3 V` and `BLK` driven to `3.3 V` (`HIGH`), measure current flowing into the `BLK` pin.
   - **Pass Criterion**: $I_{\text{BLK}} < 2\text{ mA}$ (confirming `BLK` drives the high-impedance base/gate of transistor `U2` through a series resistor, rather than sourcing the ~60 mA raw LED array directly).
2. **Pull-up / Default State Check**:
   - Observe whether `BLK` floats HIGH or LOW when disconnected, and ensure the ESP32-S3 pull-down keeps the backlight OFF during reset.
3. **Switching Frequency Selection**:
   - Because `U2` is an uncharacterized SOT-23 BJT/MOSFET with an unknown base/gate resistor (typically 1 kΩ – 10 kΩ), excessive PWM frequencies (> 10 kHz) can cause slow rise/fall transitions and thermal dissipation in the linear region.
   - Use a conservative **1 kHz – 2 kHz** PWM carrier frequency: fast enough to eliminate human-perceptible flicker (> 200 Hz) while keeping switching losses negligible.

---

## 3. Configuration & Kconfig Toggle

PWM mode is guarded by a compile-time configuration option (in `Kconfig.projbuild` or header config):

- `CONFIG_BSP_DISPLAY_BACKLIGHT_PWM` (Default: `n` until hardware check passes)
  - When **`n` (Disabled)**: `bsp_backlight` uses pure digital GPIO output (`gpio_set_level`). Calling `bsp_backlight_set_brightness(pct)` maps `0%` to `OFF` and `1–100%` to `ON`.
  - When **`y` (Enabled)**: `bsp_backlight` configures the ESP32-S3 LEDC peripheral on `GPIO 9`.

### LEDC Hardware Parameters

| Parameter | Value | Rationale |
|---|---|---|
| **GPIO Pin** | `GPIO 9` | Display Pin 8 (`BLK`) |
| **Speed Mode** | `LEDC_LOW_SPEED_MODE` | Standard on ESP32-S3, supports hardware fade |
| **Timer** | `LEDC_TIMER_0` | Dedicated timer for display backlight |
| **Channel** | `LEDC_CHANNEL_0` | Bound to `GPIO 9` |
| **PWM Frequency** | `1000 Hz` (1 kHz) | Safe switching rate for uncharacterized `U2` transistor |
| **Duty Resolution** | `LEDC_TIMER_10_BIT` (0–1023) | Smooth 1024-step dimming granularity |

---

## 4. Software Architecture & API Extension

### Updated `bsp_backlight.h`

The API preserves full backward compatibility with the binary functions from Spec 001 while adding percentage and fade controls:

```c
#pragma once
#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize backlight on GPIO 9 (defaults to OFF / 0% brightness).
 *        Uses digital GPIO or LEDC PWM depending on CONFIG_BSP_DISPLAY_BACKLIGHT_PWM.
 */
esp_err_t bsp_backlight_init(void);

/**
 * @brief Binary backlight control (Spec 001 compatible).
 *        Maps to 100% (true) or 0% (false) when PWM is enabled.
 */
esp_err_t bsp_backlight_set(bool enable);
esp_err_t bsp_backlight_on(void);
esp_err_t bsp_backlight_off(void);

/**
 * @brief Set backlight brightness level (0 to 100%).
 *        In binary mode, 0 = OFF and 1..100 = ON.
 */
esp_err_t bsp_backlight_set_brightness(uint8_t percent);

/**
 * @brief Smoothly transition backlight brightness over duration_ms using LEDC hardware fade.
 *        In binary mode, switches state immediately.
 */
esp_err_t bsp_backlight_fade_to(uint8_t percent, uint32_t duration_ms);
```

### Integration with Spec 002 (Multicore Command Queue)

When `CONFIG_BSP_DISPLAY_BACKLIGHT_PWM` is enabled, the `DISP_CMD_SET_BACKLIGHT` command in `xDisplayCmdQueue` ([`spec/002-freertos-multicore-strategy.md`](002-freertos-multicore-strategy.md)) accepts `0–100%` brightness values and optional fade durations.

---

## 5. Implementation Steps

1. **Step 1: Hardware Verification**:
   - Run Spec 001 binary bring-up and verify $I_{\text{BLK}} < 2\text{ mA}$.
2. **Step 2: Kconfig & Driver Implementation**:
   - Add `CONFIG_BSP_DISPLAY_BACKLIGHT_PWM` option.
   - Implement `ledc_timer_config()` and `ledc_channel_config()` in `bsp_backlight.c` with `duty = 0` at init.
   - Install hardware fade service via `ledc_fade_func_install(0)` and implement `bsp_backlight_fade_to()` using `ledc_set_fade_with_time()` + `ledc_fade_start(..., LEDC_FADE_NO_WAIT)`.
3. **Step 3: Dimming & Thermal Test**:
   - Test brightness steps (`0%`, `10%`, `25%`, `50%`, `75%`, `100%`) and 500 ms fade-in / fade-out transitions.

---

## 6. Acceptance & Verification Criteria

- [ ] **Prerequisite Passed**: Measured `BLK` current is confirmed $< 2\text{ mA}$ before enabling `CONFIG_BSP_DISPLAY_BACKLIGHT_PWM`.
- [ ] **Fallback Compatibility**: With `CONFIG_BSP_DISPLAY_BACKLIGHT_PWM=n`, the driver compiles and operates purely as a binary GPIO output.
- [ ] **Flicker-Free Dimming**: With `CONFIG_BSP_DISPLAY_BACKLIGHT_PWM=y` at 1 kHz, brightness steps from 5% to 100% show smooth visual luminance scaling without visible flicker or audible whine.
- [ ] **Thermal Stability**: `U2` on the display PCB remains cool to the touch during extended operation at 50% PWM duty cycle.

