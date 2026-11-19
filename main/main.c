#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "TCA9554PWR.h"
#include "I2C_Driver.h"
#include "ST7701S.h"
#include "GT911.h"
#include "LVGL_Driver.h"
#include "ui/ui.h"

/* Start the LVGL task only after building the UI, keeping LVGL initialization
 * and object creation serialized. Later UI work runs in this task's callbacks. */
extern void lvgl_task(void *arg);

void app_main(void)
{
    I2C_Init();
    EXIO_Init();
    LCD_Init();
    Touch_Init();
    Set_Backlight(0);
    LVGL_Init();
    ui_init();
    BaseType_t result = xTaskCreatePinnedToCore(lvgl_task, "lvgl_task",
                                               8192, NULL, 5, NULL, 1);
    configASSERT(result == pdPASS);
    Set_Backlight(75);
}
