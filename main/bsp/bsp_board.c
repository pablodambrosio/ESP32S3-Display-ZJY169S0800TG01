#include "bsp_board.h"

#include <inttypes.h>
#include "sdkconfig.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_log.h"

static const char *TAG = "bsp_board";

esp_err_t bsp_board_init(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint32_t flash_size_bytes = 0;
    esp_err_t ret = esp_flash_get_size(NULL, &flash_size_bytes);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to query flash size: %s", esp_err_to_name(ret));
    }

    size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    size_t dma_free = heap_caps_get_free_size(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    size_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t spiram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);

    ESP_LOGI(TAG, "==========================================================");
    ESP_LOGI(TAG, " Waveshare ESP32-S3-Zero + ZJY169S0800TG01 Bring-up (001)");
    ESP_LOGI(TAG, "==========================================================");
    ESP_LOGI(TAG, "ESP-IDF Version : %s", esp_get_idf_version());
    ESP_LOGI(TAG, "MCU Model       : ESP32-S3 (Cores: %d, Rev: v%d.%d)",
             chip_info.cores, chip_info.revision / 100, chip_info.revision % 100);
    ESP_LOGI(TAG, "CPU Frequency   : %d MHz", CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    ESP_LOGI(TAG, "Flash Size      : %" PRIu32 " MB (%s)",
             flash_size_bytes / (1024 * 1024),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    ESP_LOGI(TAG, "Embedded PSRAM  : %s (eFuse feature flag)",
             (chip_info.features & CHIP_FEATURE_EMB_PSRAM) ? "Yes" : "No");
    ESP_LOGI(TAG, "Internal SRAM   : %u KB free / %u KB total (DMA-capable free: %u KB)",
             (unsigned)(internal_free / 1024),
             (unsigned)(internal_total / 1024),
             (unsigned)(dma_free / 1024));
    if (spiram_total > 0) {
        ESP_LOGI(TAG, "External PSRAM  : %u KB free / %u KB total",
                 (unsigned)(spiram_free / 1024),
                 (unsigned)(spiram_total / 1024));
    } else {
        ESP_LOGI(TAG, "External PSRAM  : Not enabled in sdkconfig");
    }
    ESP_LOGI(TAG, "==========================================================");

    return ESP_OK;
}

