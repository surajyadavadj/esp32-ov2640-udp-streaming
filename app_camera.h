#pragma once

#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* JPEG frame container */
typedef struct {
    uint8_t *data;
    size_t   len;
} jpeg_frame_t;

/* Global queue: Camera → Ethernet */
extern QueueHandle_t g_frame_queue;

/* Camera app APIs */
void App_Camera_Init(void);
void App_Camera_StartTask(void);
