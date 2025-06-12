#ifndef CAMERA_SENDER_H
#define CAMERA_SENDER_H

#include <gst/gst.h>
#include <string>

class CameraSender {
public:
    CameraSender();
    ~CameraSender();

    bool initialize();
    bool start();
    void stop();
    bool isRunning() const { return running; }

private:
    GstElement* pipeline;
    GstElement* source;
    GstElement* converter;
    GstElement* encoder;
    GstElement* parser;
    GstElement* payloader;
    GstElement* sink;
    GMainLoop* loop;
    bool running;

    static void onEOS(GstBus* bus, GstMessage* msg, gpointer data);
    static void onError(GstBus* bus, GstMessage* msg, gpointer data);
};

#endif // CAMERA_SENDER_H 