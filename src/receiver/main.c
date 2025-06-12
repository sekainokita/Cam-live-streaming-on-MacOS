#include "video_receiver.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static VideoReceiver* g_receiver = NULL;

void signal_handler(int signum) {
    if (g_receiver) {
        video_receiver_stop(g_receiver);
    }
}

int main(int argc, char* argv[]) {
    // 시그널 핸들러 설정
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 비디오 수신기 생성 및 초기화
    g_receiver = video_receiver_new();
    if (!g_receiver) {
        fprintf(stderr, "Failed to create video receiver\n");
        return 1;
    }

    if (!video_receiver_initialize(g_receiver)) {
        fprintf(stderr, "Failed to initialize video receiver\n");
        video_receiver_free(g_receiver);
        return 1;
    }

    // 재생 시작
    if (!video_receiver_start(g_receiver)) {
        fprintf(stderr, "Failed to start video receiver\n");
        video_receiver_free(g_receiver);
        return 1;
    }

    printf("Video playback started. Press Ctrl+C to stop.\n");

    // 메인 루프
    while (video_receiver_is_running(g_receiver)) {
        g_usleep(100000);  // 100ms 대기
    }

    // 정리
    video_receiver_free(g_receiver);
    return 0;
} 