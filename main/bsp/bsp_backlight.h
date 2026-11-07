#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display backlight control GPIO (Display Pin 8: BLK -> ESP32-S3 GPIO 9)
 */
#define BSP_LCD_PIN_BLK  GPIO_NUM_9

/**
 * @brief Initialize GPIO 9 as digital output for backlight control, defaulting to OFF (0).
 *
 * @return ESP_OK on success, or an esp_err_t error code.
 */
esp_err_t bsp_backlight_init(void);

/**
 * @brief Set backlight binary state (true = HIGH / ON, false = LOW / OFF).
 *
 * @param enable true to turn backlight ON, false to turn OFF.
 * @return ESP_OK on success, or an esp_err_t error code.
 */
esp_err_t bsp_backlight_set(bool enable);

/**
 * @brief Turn display backlight ON (drives GPIO 9 HIGH).
 */
esp_err_t bsp_backlight_on(void);

/**
 * @brief Turn display backlight OFF (drives GPIO 9 LOW).
 */
esp_err_t bsp_backlight_off(void);

#ifdef __cplusplus
}
#endif

