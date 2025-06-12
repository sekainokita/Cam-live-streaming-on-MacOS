#ifndef VIDEO_RECEIVER_H
#define VIDEO_RECEIVER_H

#include <gst/gst.h>
#include <string>

class VideoReceiver {
public:
    VideoReceiver();
    ~VideoReceiver();

    bool initialize();
    bool start();
    void stop();
    bool isRunning() const { return running; }

private:
    GstElement* pipeline;
    GstElement* source;
    GstElement* depayloader;
    GstElement* decoder;
    GstElement* converter;
    GstElement* sink;
    GstElement* caps;
    GMainLoop* loop;
    bool running;

    static void onEOS(GstBus* bus, GstMessage* msg, gpointer data);
    static void onError(GstBus* bus, GstMessage* msg, gpointer data);
};

#endif // VIDEO_RECEIVER_H 