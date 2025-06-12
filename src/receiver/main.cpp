#include "video_receiver.h"
#include <iostream>
#include <csignal>
#include <gst/gst.h>
#ifdef __APPLE__
#include <gst/gstmacos.h>
#endif

static bool running = true;

void signal_handler(int signum) {
    running = false;
}

static int run_main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // GStreamer 초기화
    gst_init(&argc, &argv);

    VideoReceiver receiver;
    if (!receiver.initialize()) {
        std::cerr << "Failed to initialize video receiver" << std::endl;
        return -1;
    }

    if (!receiver.start()) {
        std::cerr << "Failed to start video receiver" << std::endl;
        return -1;
    }

    std::cout << "Receiving started. Press Ctrl+C to stop." << std::endl;

    // 메인 루프
    while (running) {
        g_usleep(100000);  // 100ms 대기
    }

    receiver.stop();
    return 0;
}

int main(int argc, char* argv[]) {
#ifdef __APPLE__
    return gst_macos_main((GstMainFunc)run_main, argc, argv, nullptr);
#else
    return run_main(argc, argv);
#endif
} 