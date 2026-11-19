/*
 * SPDX-License-Identifier: Apache-2.0
 * CST820 single-point reader for Waveshare ESP32-S3-Touch-LCD-2.1.
 * Register layout follows the CST816-compatible protocol used by Espressif's
 * esp_lcd_touch_cst816s driver. Retains GT911.c filename for existing builds.
 */
#include "GT911.h"
#include <stdlib.h>

static const char *TAG = "CST820";
esp_lcd_touch_handle_t tp = NULL;

static esp_err_t cst_read(esp_lcd_touch_handle_t touch)
{
    uint8_t data[5] = {0};
    esp_err_t err = esp_lcd_panel_io_rx_param(touch->io, 0x02, data, sizeof(data));
    taskENTER_CRITICAL(&touch->data.lock);
    /* Clear stale press data even if an I2C transaction fails. */
    touch->data.points = 0;
    if(err == ESP_OK && (data[0] & 0x0F) == 1) {
        uint16_t x = ((data[1] & 0x0F) << 8) | data[2];
        uint16_t y = ((data[3] & 0x0F) << 8) | data[4];
        if(x < touch->config.x_max && y < touch->config.y_max) {
            touch->data.points = 1;
            touch->data.coords[0].x = x;
            touch->data.coords[0].y = y;
            touch->data.coords[0].strength = 1;
        }
    }
    taskEXIT_CRITICAL(&touch->data.lock);
    return err;
}

static bool cst_get_xy(esp_lcd_touch_handle_t touch, uint16_t *x, uint16_t *y,
                       uint16_t *strength, uint8_t *count, uint8_t max_count)
{
    if(!touch || !x || !y || !count || !max_count) return false;
    taskENTER_CRITICAL(&touch->data.lock);
    *count = touch->data.points ? 1 : 0;
    if(*count) {
        x[0] = touch->data.coords[0].x;
        y[0] = touch->data.coords[0].y;
        if(strength) strength[0] = 1;
    }
    touch->data.points = 0;
    taskEXIT_CRITICAL(&touch->data.lock);
    return *count != 0;
}

static esp_err_t cst_delete(esp_lcd_touch_handle_t touch)
{
    free(touch);
    return ESP_OK;
}

void Touch_Init(void)
{
    /* Preserve the board's expander reset line; CST820 does not use GT911's
     * interrupt-pin address-selection sequence. */
    gpio_set_direction(I2C_Touch_INT_IO, GPIO_MODE_INPUT);
    Set_EXIO(TCA9554_EXIO2, false);
    vTaskDelay(pdMS_TO_TICKS(20));
    Set_EXIO(TCA9554_EXIO2, true);
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_i2c_config_t cfg = {
        .dev_addr = 0x15,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 8,
        .flags.disable_control_phase = 1,
    };
    esp_err_t err = esp_lcd_new_panel_io_i2c(
        (esp_lcd_i2c_bus_handle_t)I2C_MASTER_NUM, &cfg, &io);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Touch IO unavailable: %s", esp_err_to_name(err));
        return;
    }
    /* Keep the CST820 awake for LVGL polling (DisAutoSleep register). */
    uint8_t awake = 0xFF;
    err = esp_lcd_panel_io_tx_param(io, 0xFE, &awake, 1);
    uint8_t id = 0;
    if(err == ESP_OK) err = esp_lcd_panel_io_rx_param(io, 0xA7, &id, 1);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "No CST820 response at 0x15; touch disabled: %s",
                 esp_err_to_name(err));
        esp_lcd_panel_io_del(io);
        return;
    }
    esp_lcd_touch_handle_t touch = calloc(1, sizeof(esp_lcd_touch_t));
    if(!touch) {
        ESP_LOGE(TAG, "No memory for touch handle");
        esp_lcd_panel_io_del(io);
        return;
    }
    touch->io = io;
    touch->config.x_max = EXAMPLE_LCD_H_RES;
    touch->config.y_max = EXAMPLE_LCD_V_RES;
    touch->config.rst_gpio_num = GPIO_NUM_NC;
    touch->config.int_gpio_num = I2C_Touch_INT_IO;
    touch->data.lock.owner = portMUX_FREE_VAL;
    touch->read_data = cst_read;
    touch->get_xy = cst_get_xy;
    touch->del = cst_delete;
    tp = touch;
    ESP_LOGI(TAG, "Touch ready at 0x15, chip ID 0x%02X, %dx%d",
             id, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
}
