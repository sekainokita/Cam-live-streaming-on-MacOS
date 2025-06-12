# live Streaming Project

이 프로젝트는 RTP와 h.264 패킷을 사용하여 카메라 영상을 스트리밍하는 애플리케이션입니다.

## 기능

- 카메라 영상 캡처 및 스트리밍
- UDP를 통한 비디오 전송
- 실시간 비디오 재생

## 요구사항

### macOS
- CMake (3.10 이상)
- Xcode Command Line Tools

## 설치 방법

### macOS

1. 프로젝트 빌드:
```bash
mkdir -p build/macos
cd build/macos
cmake ../..
make
```

## 사용 방법

### 카메라 스트리밍

1. 카메라 송신기 실행:
```bash
cd build/macos
export DYLD_LIBRARY_PATH=/opt/homebrew/lib:$DYLD_LIBRARY_PATH
export GST_PLUGIN_PATH=/opt/homebrew/lib/gstreamer-1.0
./camera_sender
```

2. 비디오 수신기 실행 (새 터미널에서):
```bash
cd build/macos
export DYLD_LIBRARY_PATH=/opt/homebrew/lib:$DYLD_LIBRARY_PATH
export GST_PLUGIN_PATH=/opt/homebrew/lib/gstreamer-1.0
./video_receiver
```

### 주의사항

- 카메라 송신기와 비디오 수신기는 별도의 터미널에서 실행해야 합니다.
- 기본적으로 localhost(127.0.0.1)의 5000번 포트를 사용합니다.
- 프로그램을 종료하려면 각 터미널에서 Ctrl+C를 누르세요.

## 문제 해결

### 일반적인 문제

1. 카메라 접근 권한 문제:
   - 시스템 환경설정 > 보안 및 개인 정보 보호 > 개인 정보 보호 > 카메라에서 터미널 앱에 대한 권한을 허용하세요.

2. 비디오가 보이지 않는 경우:
   - 카메라 송신기와 비디오 수신기가 모두 실행 중인지 확인하세요.
   - 방화벽 설정에서 5000번 포트가 차단되어 있지 않은지 확인하세요.
