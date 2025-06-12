cmake_minimum_required(VERSION 3.10)
project(Cam-live-streaming-on-MacOS C)

# C 표준 설정
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# GStreamer 패키지 찾기
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

# GStreamer 경로 직접 지정
set(GSTREAMER_ROOT "C:/gstreamer/1.0/msvc_x86_64")
set(GSTREAMER_INCLUDE_DIRS 
    "${GSTREAMER_ROOT}/include"
    "${GSTREAMER_ROOT}/include/gstreamer-1.0"
    "${GSTREAMER_ROOT}/lib/gstreamer-1.0/include"
    "${GSTREAMER_ROOT}/include/glib-2.0"
    "${GSTREAMER_ROOT}/lib/glib-2.0/include"
)
set(GSTREAMER_LIBRARY_DIRS "${GSTREAMER_ROOT}/lib")
set(GSTREAMER_LIBRARIES
    "${GSTREAMER_ROOT}/lib/gstreamer-1.0.lib"
    "${GSTREAMER_ROOT}/lib/gstvideo-1.0.lib"
    "${GSTREAMER_ROOT}/lib/gstapp-1.0.lib"
    "${GSTREAMER_ROOT}/lib/glib-2.0.lib"
    "${GSTREAMER_ROOT}/lib/gobject-2.0.lib"
)

# 송신기 실행 파일
add_executable(camera_sender
    src/sender/main.c
    src/sender/camera_sender.c
)

# 수신기 실행 파일
add_executable(video_receiver
    src/receiver/main.c
    src/receiver/video_receiver.c
)

# 송신기 헤더 파일 경로 설정
target_include_directories(camera_sender PRIVATE
    ${GSTREAMER_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/sender
)

# 수신기 헤더 파일 경로 설정
target_include_directories(video_receiver PRIVATE
    ${GSTREAMER_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/receiver
)

# 송신기 링크 설정
target_link_libraries(camera_sender
    ${GSTREAMER_LIBRARIES}
)

# 수신기 링크 설정
target_link_libraries(video_receiver
    ${GSTREAMER_LIBRARIES}
)

# 컴파일러 플래그 설정
target_compile_options(camera_sender PRIVATE
    ${GSTREAMER_CFLAGS_OTHER}
)

target_compile_options(video_receiver PRIVATE
    ${GSTREAMER_CFLAGS_OTHER}
) 