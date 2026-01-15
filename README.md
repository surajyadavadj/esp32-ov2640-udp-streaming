# esp32-ov2640-udp-streaming
High-speed ESP32 OV2640 camera streaming over UDP with a C++ receiver.

# ESP32 UDP Camera Streaming (OV2640)

High-speed ESP32 OV2640 camera streaming over UDP with a C++ receiver.
Designed for **low latency**, **non-blocking**, and **stable image transfer**.

This project captures JPEG frames on ESP32, streams them via UDP,
and reassembles & saves images on a PC using a high-performance C++ receiver.

---

## ✨ Features

- 📷 ESP32 OV2640 camera capture (JPEG)
- 🚀 UDP streaming (fire-and-forget, no blocking)
- 🧵 FreeRTOS multi-tasking (Camera + WiFi separated)
- 🧠 Watchdog-safe camera task
- ⚡ High-speed C++ UDP receiver
- 🗂️ Image reassembly & safe saving
- 📊 Stable FPS control (2 FPS / 5 FPS / 10 FPS configurable)
- ❌ No TCP blocking
- ❌ No Python performance limits

---

## 📐 Architecture


---

## 🧩 ESP32 Side

### Components
- ESP-IDF
- OV2640 Camera
- FreeRTOS
- WiFi (UDP)

### Tasks
- `camera_task`  
  - Captures JPEG frames
  - FPS locked (e.g. 2 FPS / 10 FPS)
  - Watchdog safe
- `wifi_task`
  - Sends JPEG frames via UDP
  - Chunked packets (MTU safe)

---

## 💻 PC Receiver (C++)

- Raw UDP socket
- Packet reassembly
- Frame integrity check
- Optional disk saving
- Very low CPU usage

### Build

```bash
g++ udp_receiver.cpp -O3 -o udp_receiver
Run
./udp_receiver
