#ifndef VIDEO_RECEIVER_H
#define VIDEO_RECEIVER_H

#include <gst/gst.h>
#include <stdbool.h>

typedef struct {
    GstElement* pipeline;
    GstElement* sink;
    GMainLoop* loop;
    bool running;
} VideoReceiver;

// 생성자/소멸자
VideoReceiver* video_receiver_new(void);
void video_receiver_free(VideoReceiver* receiver);

// 초기화 및 제어
bool video_receiver_initialize(VideoReceiver* receiver);
bool video_receiver_start(VideoReceiver* receiver);
void video_receiver_stop(VideoReceiver* receiver);
bool video_receiver_is_running(const VideoReceiver* receiver);

// 콜백 함수
void on_eos(GstBus* bus, GstMessage* msg, gpointer data);
void on_error(GstBus* bus, GstMessage* msg, gpointer data);
void on_state_changed(GstBus* bus, GstMessage* msg, gpointer data);
void on_pad_added(GstElement* element, GstPad* pad, gpointer data);

#endif // VIDEO_RECEIVER_H 