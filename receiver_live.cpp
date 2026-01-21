#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <opencv2/opencv.hpp>

#include <iostream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

#define PORT 5000
#define MAX_PACKET 1500

// FRAME BUFFER
struct FrameBuffer {
    uint16_t total_chunks = 0;
    uint16_t received = 0;
    std::vector<std::vector<uint8_t>> chunks;
};

//  GLOBAL 
std::mutex frame_mutex;
cv::Mat latest_frame;
std::atomic<bool> running(true);

//  DISPLAY THREAD 
void display_thread()
{
    cv::namedWindow("ESP32 Live", cv::WINDOW_AUTOSIZE);

    while (running) {
        cv::Mat frame_copy;

        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            if (!latest_frame.empty())
                latest_frame.copyTo(frame_copy);
        }

        if (!frame_copy.empty()) {
            cv::imshow("ESP32 Live", frame_copy);
        }

        if (cv::waitKey(1) == 27) { // ESC
            running = false;
            break;
        }
    }

    cv::destroyAllWindows();
}

//   MAIN 
int main()
{
    //  Socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    int rcvbuf = 8 * 1024 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (sockaddr*)&addr, sizeof(addr));

    std::cout << "Waiting for UDP stream...\n";

    //   Start display thread 
    std::thread viewer(display_thread);

    std::unordered_map<uint16_t, FrameBuffer> frames;
    uint8_t buffer[MAX_PACKET];

    int fps = 0;
    auto t0 = std::chrono::steady_clock::now();

    // UDP receive loop 
    while (running) {
        ssize_t len = recvfrom(sock, buffer, sizeof(buffer), 0, nullptr, nullptr);
        if (len < 8) continue;

        uint16_t frame_id = *(uint16_t*)(buffer + 0);
        uint16_t chunk_id = *(uint16_t*)(buffer + 2);
        uint16_t total    = *(uint16_t*)(buffer + 4);
        uint16_t size     = *(uint16_t*)(buffer + 6);

        auto &fb = frames[frame_id];

        if (fb.chunks.empty()) {
            fb.total_chunks = total;
            fb.chunks.resize(total);
        }

        if (chunk_id < fb.chunks.size() && fb.chunks[chunk_id].empty()) {
            fb.chunks[chunk_id].assign(buffer + 8, buffer + 8 + size);
            fb.received++;
        }

       //  Frame complete 
        if (fb.received == fb.total_chunks) {
            std::vector<uint8_t> jpeg;
            for (auto &c : fb.chunks)
                jpeg.insert(jpeg.end(), c.begin(), c.end());

            cv::Mat img = cv::imdecode(jpeg, cv::IMREAD_COLOR);

            if (!img.empty()) {
                std::lock_guard<std::mutex> lock(frame_mutex);
                latest_frame = img;
            }

            frames.erase(frame_id);
            fps++;

            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - t0).count() >= 1) {
                std::cout << "FPS: " << fps << std::endl;
                fps = 0;
                t0 = now;
            }
        }
    }

    close(sock);
    viewer.join();
    return 0;
}
