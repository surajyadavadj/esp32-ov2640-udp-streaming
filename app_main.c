#include "app_camera.h"
#include "app_wifi_task.h"
#include "wifi_hal.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_task_wdt.h"   

void app_main(void)
{

    esp_task_wdt_deinit();   

    //  WIFI 
    WiFi_Init();

    while (!WiFi_IsConnected()) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    //  CAMERA 
    App_Camera_Init();
    App_Camera_StartTask();

    //  UDP STREAM 
    App_WiFi_StartTask();   // UDP sender
}
