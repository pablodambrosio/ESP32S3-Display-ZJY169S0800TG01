#include "bsp_display.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"

static const char *TAG = "bsp_display";

static esp_lcd_panel_handle_t s_panel_handle = NULL;
static esp_lcd_panel_io_handle_t s_io_handle = NULL;
static uint16_t *s_dma_buf = NULL;
static SemaphoreHandle_t s_dma_done_sem = NULL;

static bool IRAM_ATTR on_color_trans_done_cb(esp_lcd_panel_io_handle_t panel_io,
                                             esp_lcd_panel_io_event_data_t *edata,
                                             void *user_ctx)
{
    BaseType_t high_task_awoken = pdFALSE;
    SemaphoreHandle_t sem = (SemaphoreHandle_t)user_ctx;
    if (sem != NULL) {
        xSemaphoreGiveFromISR(sem, &high_task_awoken);
    }
    return high_task_awoken == pdTRUE;
}

static esp_err_t flush_dma_slice(int x_start, int y_start, int x_end, int y_end, const uint16_t *buf)
{
    ESP_RETURN_ON_FALSE(s_panel_handle != NULL && s_dma_done_sem != NULL,
                        ESP_ERR_INVALID_STATE, TAG, "Display not initialized");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_draw_bitmap(s_panel_handle, x_start, y_start, x_end, y_end, buf),
                        TAG, "draw_bitmap failed");

    if (xSemaphoreTake(s_dma_done_sem, pdMS_TO_TICKS(500)) != pdTRUE) {
        ESP_LOGE(TAG, "Timeout waiting for SPI DMA transfer completion");
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t bsp_display_init(esp_lcd_panel_handle_t *out_panel, esp_lcd_panel_io_handle_t *out_io)
{
    if (s_panel_handle != NULL) {
        ESP_LOGW(TAG, "Display already initialized");
        if (out_panel) {
            *out_panel = s_panel_handle;
        }
        if (out_io) {
            *out_io = s_io_handle;
        }
        return ESP_OK;
    }

    s_dma_done_sem = xSemaphoreCreateBinary();
    ESP_RETURN_ON_FALSE(s_dma_done_sem != NULL, ESP_ERR_NO_MEM, TAG, "Failed to create DMA semaphore");

    const size_t dma_buf_pixels = BSP_LCD_H_RES * BSP_LCD_DMA_LINES;
    const size_t dma_buf_bytes = dma_buf_pixels * sizeof(uint16_t);
    s_dma_buf = (uint16_t *)heap_caps_malloc(dma_buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    ESP_RETURN_ON_FALSE(s_dma_buf != NULL, ESP_ERR_NO_MEM, TAG,
                        "Failed to allocate %u bytes in internal DMA SRAM", (unsigned)dma_buf_bytes);

    ESP_LOGI(TAG, "Allocated %u bytes DMA slice buffer in internal SRAM (%d x %d lines)",
             (unsigned)dma_buf_bytes, BSP_LCD_H_RES, BSP_LCD_DMA_LINES);

    ESP_LOGI(TAG, "Initializing SPI2 (FSPI) bus (SCLK=%d, MOSI=%d, CS=%d, DC=%d, RST=%d)",
             BSP_LCD_PIN_SCLK, BSP_LCD_PIN_MOSI, BSP_LCD_PIN_CS, BSP_LCD_PIN_DC, BSP_LCD_PIN_RST);

    const spi_bus_config_t buscfg = {
        .sclk_io_num = BSP_LCD_PIN_SCLK,
        .mosi_io_num = BSP_LCD_PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (int)dma_buf_bytes,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(BSP_LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO),
                        TAG, "spi_bus_initialize failed");

    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = BSP_LCD_PIN_DC,
        .cs_gpio_num = BSP_LCD_PIN_CS,
        .pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = BSP_LCD_CMD_BITS,
        .lcd_param_bits = BSP_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = on_color_trans_done_cb,
        .user_ctx = s_dma_done_sem,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_HOST, &io_config, &s_io_handle),
                        TAG, "esp_lcd_new_panel_io_spi failed");

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(s_io_handle, &panel_config, &s_panel_handle),
                        TAG, "esp_lcd_new_panel_st7789 failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel_handle), TAG, "panel_reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel_handle), TAG, "panel_init failed");

    // Configure 240x280 active area offset inside 240x320 ST7789V2 GRAM
    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(s_panel_handle, BSP_LCD_X_GAP, BSP_LCD_Y_GAP),
                        TAG, "panel_set_gap failed");

    // Enable display inversion required for IPS panel accurate color reproduction
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(s_panel_handle, true),
                        TAG, "panel_invert_color failed");

    // Clear GRAM to black before turning panel output and backlight on
    ESP_RETURN_ON_ERROR(bsp_display_fill_color(BSP_COLOR_BLACK),
                        TAG, "Initial GRAM black clear failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel_handle, true),
                        TAG, "panel_disp_on_off failed");

    ESP_LOGI(TAG, "ST7789V2 initialized (%dx%d, gap X=%d Y=%d, SPI clk=%d MHz, IPS invert=ON)",
             BSP_LCD_H_RES, BSP_LCD_V_RES, BSP_LCD_X_GAP, BSP_LCD_Y_GAP,
             BSP_LCD_PIXEL_CLOCK_HZ / (1000 * 1000));

    if (out_panel) {
        *out_panel = s_panel_handle;
    }
    if (out_io) {
        *out_io = s_io_handle;
    }
    return ESP_OK;
}

esp_err_t bsp_display_fill_color(uint16_t color)
{
    ESP_RETURN_ON_FALSE(s_panel_handle != NULL && s_dma_buf != NULL,
                        ESP_ERR_INVALID_STATE, TAG, "Display not initialized");

    const size_t slice_pixels = BSP_LCD_H_RES * BSP_LCD_DMA_LINES;
    for (size_t i = 0; i < slice_pixels; i++) {
        s_dma_buf[i] = color;
    }

    for (int y = 0; y < BSP_LCD_V_RES; y += BSP_LCD_DMA_LINES) {
        int y_end = y + BSP_LCD_DMA_LINES;
        if (y_end > BSP_LCD_V_RES) {
            y_end = BSP_LCD_V_RES;
        }
        ESP_RETURN_ON_ERROR(flush_dma_slice(0, y, BSP_LCD_H_RES, y_end, s_dma_buf),
                            TAG, "Failed to flush solid color slice at y=%d", y);
    }

    return ESP_OK;
}

esp_err_t bsp_display_draw_test_pattern(void)
{
    ESP_RETURN_ON_FALSE(s_panel_handle != NULL && s_dma_buf != NULL,
                        ESP_ERR_INVALID_STATE, TAG, "Display not initialized");

    // 7 horizontal color bands (each 40 rows high -> 7 * 40 = 280 rows)
    static const uint16_t band_colors[7] = {
        BSP_COLOR_RED,
        BSP_COLOR_GREEN,
        BSP_COLOR_BLUE,
        BSP_COLOR_YELLOW,
        BSP_COLOR_CYAN,
        BSP_COLOR_MAGENTA,
        BSP_COLOR_BLACK,
    };

    for (int y_base = 0; y_base < BSP_LCD_V_RES; y_base += BSP_LCD_DMA_LINES) {
        int lines = BSP_LCD_DMA_LINES;
        if (y_base + lines > BSP_LCD_V_RES) {
            lines = BSP_LCD_V_RES - y_base;
        }

        for (int row = 0; row < lines; row++) {
            int abs_y = y_base + row;
            int band_idx = abs_y / 40;
            if (band_idx > 6) {
                band_idx = 6;
            }
            uint16_t row_color = band_colors[band_idx];
            uint16_t *row_ptr = &s_dma_buf[row * BSP_LCD_H_RES];

            // 1-pixel outer white perimeter border at y=0, y=279, x=0, x=239
            if (abs_y == 0 || abs_y == (BSP_LCD_V_RES - 1)) {
                for (int x = 0; x < BSP_LCD_H_RES; x++) {
                    row_ptr[x] = BSP_COLOR_WHITE;
                }
            } else {
                for (int x = 0; x < BSP_LCD_H_RES; x++) {
                    row_ptr[x] = row_color;
                }
                row_ptr[0] = BSP_COLOR_WHITE;
                row_ptr[BSP_LCD_H_RES - 1] = BSP_COLOR_WHITE;
            }
        }

        ESP_RETURN_ON_ERROR(flush_dma_slice(0, y_base, BSP_LCD_H_RES, y_base + lines, s_dma_buf),
                            TAG, "Failed to flush test pattern slice at y=%d", y_base);
    }

    ESP_LOGI(TAG, "Test pattern rendered: 7 RGB565 color bands + 1px white border (0,0)-(%d,%d)",
             BSP_LCD_H_RES - 1, BSP_LCD_V_RES - 1);
    return ESP_OK;
}

