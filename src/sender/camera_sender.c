#include "camera_sender.h"
#include <stdio.h>
#include <stdlib.h>

CameraSender* camera_sender_new(void) {
    CameraSender* sender = (CameraSender*)malloc(sizeof(CameraSender));
    if (sender) {
        sender->pipeline = NULL;
        sender->source = NULL;
        sender->converter = NULL;
        sender->encoder = NULL;
        sender->muxer = NULL;
        sender->sink = NULL;
        sender->loop = NULL;
        sender->running = false;
    }
    return sender;
}

void camera_sender_free(CameraSender* sender) {
    if (sender) {
        camera_sender_stop(sender);
        free(sender);
    }
}

bool camera_sender_initialize(CameraSender* sender) {
    if (!sender) return false;

    // GStreamer 초기화
    gst_init(NULL, NULL);

    // 파이프라인 생성
    sender->pipeline = gst_pipeline_new("camera-sender");
    if (!sender->pipeline) {
        fprintf(stderr, "Failed to create pipeline\n");
        return false;
    }

    // 요소 생성
    sender->source = gst_element_factory_make("mfvideosrc", "camera-source");
    sender->converter = gst_element_factory_make("videoconvert", "converter");
    sender->encoder = gst_element_factory_make("nvh264enc", "encoder");
    sender->muxer = gst_element_factory_make("mpegtsmux", "muxer");
    sender->sink = gst_element_factory_make("filesink", "sink");

    if (!sender->source || !sender->converter || !sender->encoder || 
        !sender->muxer || !sender->sink) {
        fprintf(stderr, "Failed to create elements\n");
        return false;
    }

    // 파이프라인에 요소 추가
    gst_bin_add_many(GST_BIN(sender->pipeline), sender->source, sender->converter,
                     sender->encoder, sender->muxer, sender->sink, NULL);

    // 요소 연결
    if (!gst_element_link_many(sender->source, sender->converter, sender->encoder,
                              sender->muxer, sender->sink, NULL)) {
        fprintf(stderr, "Failed to link elements\n");
        return false;
    }

    // 카메라 소스 설정
    g_object_set(G_OBJECT(sender->source), "device-name", "HD 4MP WEBCAM", NULL);

    // 카메라 해상도 설정 (16:9 비율 유지)
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, 1920,
        "height", G_TYPE_INT, 1080,
        "framerate", GST_TYPE_FRACTION, 30, 1,
        "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
        NULL);
    g_object_set(G_OBJECT(sender->source), "caps", caps, NULL);
    gst_caps_unref(caps);

    // NVIDIA 인코더 설정
    g_object_set(G_OBJECT(sender->encoder), "preset", 4, NULL);  // low latency
    g_object_set(G_OBJECT(sender->encoder), "bitrate", 4000, NULL);  // 4000 kbps

    // 파일 싱크 설정
    g_object_set(G_OBJECT(sender->sink), "location", "output.ts", NULL);
    g_object_set(G_OBJECT(sender->sink), "sync", FALSE, NULL);
    g_object_set(G_OBJECT(sender->sink), "async", FALSE, NULL);

    return true;
}

bool camera_sender_start(CameraSender* sender) {
    if (!sender || sender->running) return false;

    // 버스 메시지 핸들러 설정
    GstBus* bus = gst_element_get_bus(sender->pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::eos", G_CALLBACK(on_eos), sender);
    g_signal_connect(bus, "message::error", G_CALLBACK(on_error), sender);
    gst_object_unref(bus);

    // 파이프라인 시작
    GstStateChangeReturn ret = gst_element_set_state(sender->pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        fprintf(stderr, "Failed to start pipeline\n");
        return false;
    }

    sender->running = true;
    return true;
}

void camera_sender_stop(CameraSender* sender) {
    if (!sender || !sender->running) return;

    gst_element_set_state(sender->pipeline, GST_STATE_NULL);
    sender->running = false;
}

bool camera_sender_is_running(const CameraSender* sender) {
    return sender ? sender->running : false;
}

void on_eos(GstBus* bus, GstMessage* msg, gpointer data) {
    CameraSender* sender = (CameraSender*)data;
    printf("End of stream\n");
    camera_sender_stop(sender);
}

void on_error(GstBus* bus, GstMessage* msg, gpointer data) {
    CameraSender* sender = (CameraSender*)data;
    GError* err;
    gchar* debug_info;
    
    gst_message_parse_error(msg, &err, &debug_info);
    fprintf(stderr, "Error received from element %s: %s\n", 
            GST_OBJECT_NAME(msg->src), err->message);
    fprintf(stderr, "Debugging information: %s\n", 
            debug_info ? debug_info : "none");
    
    g_clear_error(&err);
    g_free(debug_info);
    
    camera_sender_stop(sender);
} 