#include "camera_sender.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static CameraSender* g_sender = NULL;

void signal_handler(int signum) {
    if (g_sender) {
        camera_sender_stop(g_sender);
    }
}

int main(int argc, char* argv[]) {
    // 시그널 핸들러 설정
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 카메라 송신기 생성 및 초기화
    g_sender = camera_sender_new();
    if (!g_sender) {
        fprintf(stderr, "Failed to create camera sender\n");
        return 1;
    }

    if (!camera_sender_initialize(g_sender)) {
        fprintf(stderr, "Failed to initialize camera sender\n");
        camera_sender_free(g_sender);
        return 1;
    }

    // 스트리밍 시작
    if (!camera_sender_start(g_sender)) {
        fprintf(stderr, "Failed to start camera sender\n");
        camera_sender_free(g_sender);
        return 1;
    }

    printf("Camera streaming started. Press Ctrl+C to stop.\n");

    // 메인 루프
    while (camera_sender_is_running(g_sender)) {
        g_usleep(100000);  // 100ms 대기
    }

    // 정리
    camera_sender_free(g_sender);
    return 0;
} 