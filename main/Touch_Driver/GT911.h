/* CST820 touch interface. Legacy filename retained for existing includes. */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch/esp_lcd_touch.h"
#include "TCA9554PWR.h"
#include "I2C_Driver.h"
#include "ST7701S.h"

#define I2C_Touch_INT_IO 16
#define I2C_Touch_RST_IO -1
extern esp_lcd_touch_handle_t tp;
void Touch_Init(void);
