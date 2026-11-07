#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "esp_lcd_types.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display geometry and ST7789V2 GRAM offsets (Portrait 0 deg)
 */
#define BSP_LCD_H_RES           240
#define BSP_LCD_V_RES           280
#define BSP_LCD_X_GAP           0
#define BSP_LCD_Y_GAP           20

/**
 * @brief SPI2 (FSPI) bus and pin mapping (Waveshare ESP32-S3-Zero -> ZJY169S0800TG01)
 */
#define BSP_LCD_SPI_HOST        SPI2_HOST
#define BSP_LCD_PIN_SCLK        GPIO_NUM_4
#define BSP_LCD_PIN_MOSI        GPIO_NUM_6
#define BSP_LCD_PIN_RST         GPIO_NUM_5
#define BSP_LCD_PIN_DC          GPIO_NUM_7
#define BSP_LCD_PIN_CS          GPIO_NUM_8

/**
 * @brief SPI pixel clock and DMA slice height
 */
#define BSP_LCD_PIXEL_CLOCK_HZ  (40 * 1000 * 1000)
#define BSP_LCD_CMD_BITS        8
#define BSP_LCD_PARAM_BITS      8
#define BSP_LCD_DMA_LINES       20

/**
 * @brief Standard 16-bit RGB565 color definitions
 */
#define BSP_COLOR_BLACK         0x0000
#define BSP_COLOR_WHITE         0xFFFF
#define BSP_COLOR_RED           0xF800
#define BSP_COLOR_GREEN         0x07E0
#define BSP_COLOR_BLUE          0x001F
#define BSP_COLOR_YELLOW        0xFFE0
#define BSP_COLOR_CYAN          0x07FF
#define BSP_COLOR_MAGENTA       0xF81F

/**
 * @brief Initialize SPI2 DMA bus and ST7789V2 display panel.
 *        Applies (X=0, Y=20) offset, enables IPS color inversion, clears GRAM to black,
 *        and turns the panel display output on.
 *
 * @param[out] out_panel Optional pointer to receive the initialized panel handle.
 * @param[out] out_io    Optional pointer to receive the initialized panel IO handle.
 * @return ESP_OK on success, or an esp_err_t error code.
 */
esp_err_t bsp_display_init(esp_lcd_panel_handle_t *out_panel, esp_lcd_panel_io_handle_t *out_io);

/**
 * @brief Fill the entire 240x280 visible display area with a solid RGB565 color using DMA.
 *
 * @param color 16-bit RGB565 color value.
 * @return ESP_OK on success, or an esp_err_t error code.
 */
esp_err_t bsp_display_fill_color(uint16_t color);

/**
 * @brief Draw the Spec 001 bring-up test pattern:
 *        - 7 horizontal color bars across the 240x280 screen
 *        - 1-pixel white outer perimeter border around (0, 0) to (239, 279)
 *          to visually verify all 4 edges and confirm zero clipping.
 *
 * @return ESP_OK on success, or an esp_err_t error code.
 */
esp_err_t bsp_display_draw_test_pattern(void);

#ifdef __cplusplus
}
#endif

