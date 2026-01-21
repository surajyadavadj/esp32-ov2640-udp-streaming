#include "app_wifi_task.h"
#include "app_camera.h"
#include "wifi_hal.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "APP_WIFI";

// PC IP & PORT 
#define SERVER_IP   "192.168.0.135"
#define SERVER_PORT 5000

#define UDP_CHUNK_SIZE 1400

//  UDP HEADER 
typedef struct {
    uint16_t frame_id;
    uint16_t chunk_id;
    uint16_t total_chunks;
    uint16_t payload_size;
} __attribute__((packed)) udp_hdr_t;


static void wifi_task(void *arg)
{
    jpeg_frame_t frame;
    uint16_t frame_id = 0;

    //   Wait for WiFi 
    while (!WiFi_IsConnected()) {
        ESP_LOGI(TAG, "Waiting for WiFi...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "WiFi connected");

    // UDP socket 
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    struct sockaddr_in dest = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
        .sin_addr.s_addr = inet_addr(SERVER_IP),
    };

    ESP_LOGI(TAG, "UDP streaming started");

    while (1) {
        if (xQueueReceive(g_frame_queue, &frame, portMAX_DELAY)) {

            uint16_t total_chunks =
                (frame.len + UDP_CHUNK_SIZE - 1) / UDP_CHUNK_SIZE;

            for (uint16_t i = 0; i < total_chunks; i++) {

                uint8_t packet[1500];

                uint16_t payload =
                    (i == total_chunks - 1)
                        ? (frame.len - i * UDP_CHUNK_SIZE)
                        : UDP_CHUNK_SIZE;

                udp_hdr_t hdr = {
                    .frame_id = frame_id,
                    .chunk_id = i,
                    .total_chunks = total_chunks,
                    .payload_size = payload
                };

                memcpy(packet, &hdr, sizeof(hdr));
                memcpy(packet + sizeof(hdr),
                       frame.data + i * UDP_CHUNK_SIZE,
                       payload);

                sendto(sock,
                       packet,
                       sizeof(hdr) + payload,
                       0,
                       (struct sockaddr *)&dest,
                       sizeof(dest));
            }

            frame_id++;
            free(frame.data);   
        }
    }
}

void App_WiFi_StartTask(void)
{
    xTaskCreatePinnedToCore(
        wifi_task,
        "wifi_udp_task",
        8192,
        NULL,
        5,
        NULL,
        1   
    );
}
