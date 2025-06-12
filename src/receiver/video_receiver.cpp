#include "video_receiver.h"
#include <iostream>

VideoReceiver::VideoReceiver() : 
    pipeline(nullptr), source(nullptr), depayloader(nullptr),
    decoder(nullptr), converter(nullptr), sink(nullptr), 
    loop(nullptr), running(false) {
}

VideoReceiver::~VideoReceiver() {
    stop();
}

bool VideoReceiver::initialize() {
    // GStreamer 초기화
    gst_init(nullptr, nullptr);

    // 파이프라인 생성
    pipeline = gst_pipeline_new("video-receiver");
    if (!pipeline) {
        std::cerr << "Failed to create pipeline" << std::endl;
        return false;
    }

    // 요소 생성
    source = gst_element_factory_make("udpsrc", "source");
    depayloader = gst_element_factory_make("rtph264depay", "depayloader");
    decoder = gst_element_factory_make("avdec_h264", "decoder");
    converter = gst_element_factory_make("videoconvert", "converter");
    sink = gst_element_factory_make("osxvideosink", "sink");
    caps = gst_element_factory_make("capsfilter", "caps");

    if (!source || !depayloader || !decoder || !converter || !sink || !caps) {
        std::cerr << "Failed to create elements" << std::endl;
        return false;
    }

    // UDP 소스 설정
    g_object_set(G_OBJECT(source), "port", 5000, nullptr);
    g_object_set(G_OBJECT(source), "buffer-size", 65536, nullptr);

    // RTP capabilities 설정
    GstCaps* rtp_caps = gst_caps_new_simple("application/x-rtp",
        "media", G_TYPE_STRING, "video",
        "clock-rate", G_TYPE_INT, 90000,
        "encoding-name", G_TYPE_STRING, "H264",
        "payload", G_TYPE_INT, 96,
        nullptr);
    g_object_set(G_OBJECT(depayloader), "caps", rtp_caps, nullptr);
    gst_caps_unref(rtp_caps);

    // 비디오 싱크 설정
    g_object_set(G_OBJECT(sink), "sync", FALSE, nullptr);
    g_object_set(G_OBJECT(sink), "async", FALSE, nullptr);
    
    // 비디오 해상도 설정
    GstCaps* sink_caps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, 1920,
        "height", G_TYPE_INT, 1080,
        "framerate", GST_TYPE_FRACTION, 30, 1,
        "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
        nullptr);
    g_object_set(G_OBJECT(sink), "caps", sink_caps, nullptr);
    gst_caps_unref(sink_caps);

    // 파이프라인에 요소 추가
    gst_bin_add_many(GST_BIN(pipeline), source, caps, depayloader, decoder, converter, sink, nullptr);

    // 요소 연결
    if (!gst_element_link_many(source, caps, depayloader, decoder, converter, sink, nullptr)) {
        std::cerr << "Failed to link elements" << std::endl;
        return false;
    }

    return true;
}

bool VideoReceiver::start() {
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

void VideoReceiver::stop() {
    if (!running) return;

    // 파이프라인 정지
    gst_element_set_state(pipeline, GST_STATE_NULL);
    running = false;
}

void VideoReceiver::onEOS(GstBus* bus, GstMessage* msg, gpointer data) {
    VideoReceiver* receiver = static_cast<VideoReceiver*>(data);
    std::cout << "End of stream" << std::endl;
    receiver->stop();
}

void VideoReceiver::onError(GstBus* bus, GstMessage* msg, gpointer data) {
    VideoReceiver* receiver = static_cast<VideoReceiver*>(data);
    GError* err;
    gchar* debug_info;
    
    gst_message_parse_error(msg, &err, &debug_info);
    std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src) << ": " 
              << err->message << std::endl;
    std::cerr << "Debugging information: " << (debug_info ? debug_info : "none") << std::endl;
    
    g_clear_error(&err);
    g_free(debug_info);
    
    receiver->stop();
} 