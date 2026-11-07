#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize board-level diagnostics and print system/memory summary.
 *
 * @return ESP_OK on success, or an esp_err_t error code.
 */
esp_err_t bsp_board_init(void);

#ifdef __cplusplus
}
#endif

