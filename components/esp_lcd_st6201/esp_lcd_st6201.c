/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc/soc_caps.h"
#include "esp_check.h"
#include "esp_lcd_types.h"

#include "esp_lcd_st6201.h"
#include "esp_lcd_st6201_interface.h"

static const char *TAG = "st6201";

esp_err_t esp_lcd_new_panel_st6201(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config,
                                   esp_lcd_panel_handle_t *ret_panel)
{
    ESP_LOGI(TAG, "version: %d.%d.%d", ESP_LCD_ST6201_VER_MAJOR, ESP_LCD_ST6201_VER_MINOR, ESP_LCD_ST6201_VER_PATCH);
    ESP_RETURN_ON_FALSE(panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, TAG, "Invalid arguments");

    esp_err_t ret = esp_lcd_new_panel_st6201_general(io, panel_dev_config, ret_panel);

    return ret;
}
