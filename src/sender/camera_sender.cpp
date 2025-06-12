#include "camera_sender.h"
#include <iostream>

CameraSender::CameraSender() : pipeline(nullptr), source(nullptr), 
    encoder(nullptr), sink(nullptr), loop(nullptr), running(false) {
}

CameraSender::~CameraSender() {
    stop();
}

bool CameraSender::initialize() {
    // GStreamer 초기화
    gst_init(nullptr, nullptr);

    // 파이프라인 생성
    pipeline = gst_pipeline_new("camera-sender");
    if (!pipeline) {
        std::cerr << "Failed to create pipeline" << std::endl;
        return false;
    }

    // 요소 생성
    source = gst_element_factory_make("avfvideosrc", "camera-source");
    converter = gst_element_factory_make("videoconvert", "converter");
    encoder = gst_element_factory_make("x264enc", "encoder");
    parser = gst_element_factory_make("h264parse", "parser");
    payloader = gst_element_factory_make("rtph264pay", "payloader");
    sink = gst_element_factory_make("udpsink", "sink");

    if (!source || !converter || !encoder || !parser || !payloader || !sink) {
        std::cerr << "Failed to create elements" << std::endl;
        return false;
    }

    // 파이프라인에 요소 추가
    gst_bin_add_many(GST_BIN(pipeline), source, converter, encoder, parser, payloader, sink, nullptr);

    // 요소 연결
    if (!gst_element_link_many(source, converter, encoder, parser, payloader, sink, nullptr)) {
        std::cerr << "Failed to link elements" << std::endl;
        return false;
    }

    // 카메라 소스 설정
    g_object_set(G_OBJECT(source), "device-index", 0, nullptr);
    g_object_set(G_OBJECT(source), "do-timestamp", TRUE, nullptr);
    
    // 카메라 해상도 설정 (16:9 비율 유지)
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, 1920,
        "height", G_TYPE_INT, 1080,
        "framerate", GST_TYPE_FRACTION, 30, 1,
        "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
        nullptr);
    g_object_set(G_OBJECT(source), "caps", caps, nullptr);
    gst_caps_unref(caps);

    // 인코더 설정
    g_object_set(G_OBJECT(encoder), "tune", 0x00000004, nullptr);  // zerolatency
    g_object_set(G_OBJECT(encoder), "speed-preset", 1, nullptr);   // ultrafast
    g_object_set(G_OBJECT(encoder), "bitrate", 4000, nullptr);     // 4000 kbps for 1080p
    g_object_set(G_OBJECT(encoder), "key-int-max", 30, nullptr);   // keyframe every 30 frames

    // RTP 패이로더 설정
    g_object_set(G_OBJECT(payloader), "pt", 96, nullptr);
    g_object_set(G_OBJECT(payloader), "config-interval", 1, nullptr);

    // UDP 싱크 설정
    g_object_set(G_OBJECT(sink), "host", "127.0.0.1", nullptr);
    g_object_set(G_OBJECT(sink), "port", 5000, nullptr);
    g_object_set(G_OBJECT(sink), "sync", FALSE, nullptr);
    g_object_set(G_OBJECT(sink), "async", FALSE, nullptr);

    return true;
}

bool CameraSender::start() {
    if (running) return true;

    // 버스 메시지 핸들러 설정
    GstBus* bus = gst_element_get_bus(pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::eos", G_CALLBACK(onEOS), this);
    g_signal_connect(bus, "message::error", G_CALLBACK(onError), this);
    gst_object_unref(bus);

    // 파이프라인 시작
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to start pipeline" << std::endl;
        return false;
    }

    running = true;
    return true;
}

void CameraSender::stop() {
    if (!running) return;

    // 파이프라인 정지
    gst_element_set_state(pipeline, GST_STATE_NULL);
    running = false;
}

void CameraSender::onEOS(GstBus* bus, GstMessage* msg, gpointer data) {
    CameraSender* sender = static_cast<CameraSender*>(data);
    std::cout << "End of stream" << std::endl;
    sender->stop();
}

void CameraSender::onError(GstBus* bus, GstMessage* msg, gpointer data) {
    CameraSender* sender = static_cast<CameraSender*>(data);
    GError* err;
    gchar* debug_info;
    
    gst_message_parse_error(msg, &err, &debug_info);
    std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src) << ": " 
              << err->message << std::endl;
    std::cerr << "Debugging information: " << (debug_info ? debug_info : "none") << std::endl;
    
    g_clear_error(&err);
    g_free(debug_info);
    
    sender->stop();
} 