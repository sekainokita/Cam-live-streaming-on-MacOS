#include "ts_player.hpp"
#include <iostream>
#include <string>
#include <csignal>
#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

int _getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

int _kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}
#endif
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>

static bool running = true;

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received." << std::endl;
    running = false;
}

void printHelp() {
    std::cout << "\nControls:" << std::endl;
    std::cout << "  Space: Pause/Resume" << std::endl;
    std::cout << "  +/-: Volume up/down" << std::endl;
    std::cout << "  Left/Right: Seek backward/forward" << std::endl;
    std::cout << "  1-9: Set playback rate (1.0-9.0)" << std::endl;
    std::cout << "  Q: Quit" << std::endl;
    std::cout << "  H: Show this help" << std::endl;
}

std::string formatTime(gint64 time_ns) {
    gint64 seconds = time_ns / GST_SECOND;
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;
    
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << hours << ":"
       << std::setfill('0') << std::setw(2) << minutes << ":"
       << std::setfill('0') << std::setw(2) << seconds;
    return ss.str();
}

void printProgress(const TSPlayer& player) {
    gint64 position = player.getPosition();
    gint64 duration = player.getDuration();
    double progress = (duration > 0) ? (double)position / duration * 100 : 0;
    
    std::cout << "\r" << formatTime(position) << " / " << formatTime(duration)
              << " (" << std::fixed << std::setprecision(1) << progress << "%)"
              << " [Rate: " << player.getPlaybackRate() << "x]"
              << " [Vol: " << (int)(player.getVolume() * 100) << "%]"
              << std::flush;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <ts_file>" << std::endl;
        return 1;
    }

    // 시그널 핸들러 설정
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // TS 플레이어 생성 및 초기화
    TSPlayer player;
    if (!player.initialize()) {
        std::cerr << "Failed to initialize player" << std::endl;
        return 1;
    }

    // 파일 로드
    if (!player.loadFile(argv[1])) {
        std::cerr << "Failed to load file: " << argv[1] << std::endl;
        return 1;
    }

    // 재생 시작
    if (!player.start()) {
        std::cerr << "Failed to start playback" << std::endl;
        return 1;
    }

    std::cout << "Playing: " << argv[1] << std::endl;
    printHelp();

    // 메인 루프
    while (running) {
        if (_kbhit()) {
            char key = _getch();
            switch (key) {
                case ' ':
                    if (player.isPlaying()) {
                        player.pause();
                        std::cout << "\nPaused" << std::endl;
                    } else {
                        player.resume();
                        std::cout << "\nResumed" << std::endl;
                    }
                    break;
                case '+':
                    player.setVolume(std::min(1.0, player.getVolume() + 0.1));
                    break;
                case '-':
                    player.setVolume(std::max(0.0, player.getVolume() - 0.1));
                    break;
                case 75: // Left arrow
                    player.seek(std::max(0LL, player.getPosition() - 5 * GST_SECOND));
                    break;
                case 77: // Right arrow
                    player.seek(std::min(player.getDuration(), player.getPosition() + 5 * GST_SECOND));
                    break;
                case '1'...'9':
                    player.setPlaybackRate(key - '0');
                    break;
                case 'q':
                case 'Q':
                    running = false;
                    break;
                case 'h':
                case 'H':
                    printHelp();
                    break;
            }
        }
        printProgress(player);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 재생 중지
    player.stop();
    std::cout << "\nPlayback stopped" << std::endl;

    return 0;
} 