#include "app_camera.h"
#include "camera_hal.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include <stdlib.h>
#include <string.h>

static const char *TAG = "APP_CAMERA";

/* Frame queue (camera → wifi) */
QueueHandle_t g_frame_queue = NULL;

/* =====================================================
 * Camera Capture Task (CPU1, 10 FPS, WDT safe)
 * ===================================================== */
static void camera_task(void *arg)
{
    uint8_t *buf = NULL;
    size_t len = 0;
    bool warmup = true;

    esp_task_wdt_delete(NULL);   // WDT safe

    const TickType_t frame_interval = pdMS_TO_TICKS(500); // ✅ 2 FPS
    TickType_t last = xTaskGetTickCount();

    while (1) {

        TickType_t now = xTaskGetTickCount();
        if ((now - last) < frame_interval) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }
        last = now;

        if (Camera_CaptureFrame(&buf, &len) != CAM_OK)
            continue;

        if (warmup) {
            Camera_ReleaseFrame(buf);
            warmup = false;
            continue;
        }

        jpeg_frame_t frame;
        frame.len = len;
        frame.data = malloc(len);
        if (!frame.data) {
            Camera_ReleaseFrame(buf);
            continue;
        }

        memcpy(frame.data, buf, len);
        Camera_ReleaseFrame(buf);

        if (xQueueSend(g_frame_queue, &frame, 0) != pdTRUE) {
            free(frame.data);
        } else {
            ESP_LOGI("APP_CAMERA", "JPEG sent, size=%d", len);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}


/* =====================================================
 * Camera Init
 * ===================================================== */
void App_Camera_Init(void)
{
    g_frame_queue = xQueueCreate(2, sizeof(jpeg_frame_t));
    if (!g_frame_queue) {
        ESP_LOGE(TAG, "Queue create failed");
        return;
    }

    if (Camera_Init() != CAM_OK) {
        ESP_LOGE(TAG, "Camera init failed");
        return;
    }

    Camera_Start();
    ESP_LOGI(TAG, "Camera initialized");
}

/* =====================================================
 * Start Camera Task (PIN TO CPU1)
 * ===================================================== */
void App_Camera_StartTask(void)
{
    xTaskCreatePinnedToCore(
        camera_task,
        "camera_task",
        8192,
        NULL,
        5,
        NULL,
        1   // CPU1
    );
}
