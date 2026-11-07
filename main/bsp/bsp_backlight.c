#include "bsp_backlight.h"

#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "bsp_backlight";

esp_err_t bsp_backlight_init(void)
{
    // Ensure output register is latched LOW before enabling output driver
    ESP_RETURN_ON_ERROR(gpio_set_level(BSP_LCD_PIN_BLK, 0), TAG, "Failed to preset BLK low");

    const gpio_config_t bk_gpio_config = {
        .pin_bit_mask = (1ULL << BSP_LCD_PIN_BLK),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_RETURN_ON_ERROR(gpio_config(&bk_gpio_config), TAG, "Failed to configure BLK GPIO");
    ESP_RETURN_ON_ERROR(gpio_set_level(BSP_LCD_PIN_BLK, 0), TAG, "Failed to drive BLK low");

    ESP_LOGI(TAG, "Backlight initialized on GPIO %d (binary mode, initial state: OFF)", BSP_LCD_PIN_BLK);
    return ESP_OK;
}

esp_err_t bsp_backlight_set(bool enable)
{
    ESP_RETURN_ON_ERROR(gpio_set_level(BSP_LCD_PIN_BLK, enable ? 1 : 0),
                        TAG, "Failed to set BLK GPIO level");
    ESP_LOGI(TAG, "Backlight state -> %s (GPIO %d = %d)",
             enable ? "ON" : "OFF", BSP_LCD_PIN_BLK, enable ? 1 : 0);
    return ESP_OK;
}

esp_err_t bsp_backlight_on(void)
{
    return bsp_backlight_set(true);
}

esp_err_t bsp_backlight_off(void)
{
    return bsp_backlight_set(false);
}

