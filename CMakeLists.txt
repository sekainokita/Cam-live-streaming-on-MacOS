cmake_minimum_required(VERSION 3.10)
project(gstreamer_streaming)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GStreamer 패키지 찾기
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

# GStreamer 라이브러리 경로 설정
link_directories(${GSTREAMER_LIBRARY_DIRS})
include_directories(${GSTREAMER_INCLUDE_DIRS})

# 송신기 타겟
add_executable(camera_sender
    src/sender/main.cpp
    src/sender/camera_sender.cpp
)

# 수신기 타겟
add_executable(video_receiver
    src/receiver/main.cpp
    src/receiver/video_receiver.cpp
)

# TS 플레이어 타겟
add_executable(ts_player
    src/player/ts_player_main.cpp
    src/player/ts_player.cpp
)

# 각 타겟에 GStreamer 링크
target_link_libraries(camera_sender ${GSTREAMER_LIBRARIES})
target_link_libraries(video_receiver ${GSTREAMER_LIBRARIES})
target_link_libraries(ts_player ${GSTREAMER_LIBRARIES})

# 헤더 파일 경로 추가
target_include_directories(camera_sender PRIVATE ${GSTREAMER_INCLUDE_DIRS} src/sender)
target_include_directories(video_receiver PRIVATE ${GSTREAMER_INCLUDE_DIRS} src/receiver)
target_include_directories(ts_player PRIVATE ${GSTREAMER_INCLUDE_DIRS} src/player) 