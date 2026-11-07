#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "bsp_board.h"
#include "bsp_backlight.h"
#include "bsp_display.h"

static const char *TAG = "main_bringup";

typedef struct {
    const char *name;
    uint16_t color;
    uint32_t hold_ms;
} color_step_t;

void app_main(void)
{
    // Phase 1: System & Board Diagnostics
    ESP_ERROR_CHECK(bsp_board_init());

    // Phase 2: Initialize Backlight (Binary OFF while display initializes)
    ESP_ERROR_CHECK(bsp_backlight_init());

    // Phase 3: Initialize SPI2 DMA & ST7789V2 Panel (clears GRAM to black)
    ESP_ERROR_CHECK(bsp_display_init(NULL, NULL));

    // Enable Backlight now that GRAM contains clean black pixels
    ESP_ERROR_CHECK(bsp_backlight_on());

    // Phase 4: Visual Bring-up Color Sequence
    static const color_step_t color_sequence[] = {
        { "RED   (0xF800)", BSP_COLOR_RED,   800 },
        { "GREEN (0x07E0)", BSP_COLOR_GREEN, 800 },
        { "BLUE  (0x001F)", BSP_COLOR_BLUE,  800 },
        { "WHITE (0xFFFF)", BSP_COLOR_WHITE, 800 },
        { "BLACK (0x0000)", BSP_COLOR_BLACK, 500 },
    };

    for (size_t i = 0; i < sizeof(color_sequence) / sizeof(color_sequence[0]); i++) {
        ESP_LOGI(TAG, "Bring-up solid fill [%u/%u]: %s",
                 (unsigned)(i + 1),
                 (unsigned)(sizeof(color_sequence) / sizeof(color_sequence[0])),
                 color_sequence[i].name);
        ESP_ERROR_CHECK(bsp_display_fill_color(color_sequence[i].color));
        vTaskDelay(pdMS_TO_TICKS(color_sequence[i].hold_ms));
    }

    // Draw persistent color bands + 1-pixel outer alignment border (0,0)-(239,279)
    ESP_ERROR_CHECK(bsp_display_draw_test_pattern());
    ESP_LOGI(TAG, "Spec 001 bring-up complete. Verify all 4 white border edges on display.");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
