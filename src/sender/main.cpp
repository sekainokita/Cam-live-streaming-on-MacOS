#include "camera_sender.h"
#include <iostream>
#include <csignal>

static CameraSender* sender = nullptr;

void signalHandler(int signum) {
    if (sender) {
        std::cout << "Interrupt signal (" << signum << ") received.\n";
        sender->stop();
    }
    exit(signum);
}

int main(int argc, char* argv[]) {
    // 시그널 핸들러 설정
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // 카메라 송신기 생성 및 초기화
    sender = new CameraSender();
    if (!sender->initialize()) {
        std::cerr << "Failed to initialize camera sender" << std::endl;
        delete sender;
        return 1;
    }

    // 스트리밍 시작
    if (!sender->start()) {
        std::cerr << "Failed to start streaming" << std::endl;
        delete sender;
        return 1;
    }

    std::cout << "Streaming started. Press Ctrl+C to stop." << std::endl;

    // 메인 루프
    GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(loop);

    // 정리
    g_main_loop_unref(loop);
    delete sender;

    return 0;
} 