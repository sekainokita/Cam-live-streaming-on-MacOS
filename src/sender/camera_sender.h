#ifndef CAMERA_SENDER_H
#define CAMERA_SENDER_H

#include <gst/gst.h>
#include <stdbool.h>

typedef struct {
    GstElement* pipeline;
    GstElement* source;
    GstElement* converter;
    GstElement* encoder;
    GstElement* muxer;
    GstElement* sink;
    GMainLoop* loop;
    bool running;
} CameraSender;

// 생성자/소멸자
CameraSender* camera_sender_new(void);
void camera_sender_free(CameraSender* sender);

// 초기화 및 제어
bool camera_sender_initialize(CameraSender* sender);
bool camera_sender_start(CameraSender* sender);
void camera_sender_stop(CameraSender* sender);
bool camera_sender_is_running(const CameraSender* sender);

// 콜백 함수
void on_eos(GstBus* bus, GstMessage* msg, gpointer data);
void on_error(GstBus* bus, GstMessage* msg, gpointer data);

#endif // CAMERA_SENDER_H 