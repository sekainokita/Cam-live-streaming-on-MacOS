#include "video_receiver.h"
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

VideoReceiver* video_receiver_new(void) {
    VideoReceiver* receiver = (VideoReceiver*)malloc(sizeof(VideoReceiver));
    if (receiver) {
        receiver->pipeline = NULL;
        receiver->sink = NULL;
        receiver->loop = NULL;
        receiver->running = false;
    }
    return receiver;
}

void video_receiver_free(VideoReceiver* receiver) {
    if (receiver) {
        video_receiver_stop(receiver);
        free(receiver);
    }
}

bool video_receiver_initialize(VideoReceiver* receiver) {
    if (!receiver) return false;

    // GStreamer 초기화
    gst_init(NULL, NULL);

    // 파이프라인 생성
    receiver->pipeline = gst_pipeline_new("video-receiver");
    if (!receiver->pipeline) {
        fprintf(stderr, "Failed to create pipeline\n");
        return false;
    }

    // 요소 생성
    GstElement* source = gst_element_factory_make("filesrc", "source");
    GstElement* demuxer = gst_element_factory_make("tsdemux", "demuxer");
    GstElement* h264parse = gst_element_factory_make("h264parse", "h264parse");
    GstElement* decoder = NULL;
    GstElement* converter = gst_element_factory_make("videoconvert", "converter");
    GstElement* sink = gst_element_factory_make("autovideosink", "sink");

    if (!source || !demuxer || !h264parse || !converter || !sink) {
        fprintf(stderr, "Failed to create elements\n");
        return false;
    }

    // NVIDIA 디코더 사용 시도
    decoder = gst_element_factory_make("nvh264dec", "decoder");
    if (decoder) {
        printf("NVIDIA H264 디코더를 사용합니다.\n");
    } else {
        printf("NVIDIA H264 디코더를 사용할 수 없습니다. 소프트웨어 디코더로 대체합니다.\n");
        decoder = gst_element_factory_make("avdec_h264", "decoder");
        if (!decoder) {
            fprintf(stderr, "Failed to create decoder\n");
            return false;
        }
    }

    // 파일 소스 설정 (현재 디렉토리와 미리 준비된 경로 모두 시도)
    gchar* file_path = NULL;
    bool file_found = false;

    // 1. 현재 디렉토리에서 시도
    gchar* current_dir = g_get_current_dir();
    file_path = g_build_filename(current_dir, "output.ts", NULL);
    
    if (g_file_test(file_path, G_FILE_TEST_EXISTS)) {
        printf("현재 디렉토리에서 파일을 찾았습니다: %s\n", file_path);
        file_found = true;
    }
    
    // 2. 현재 디렉토리에서 찾지 못한 경우 미리 준비된 경로 시도
    if (!file_found) {
        g_free(file_path);
        file_path = g_strdup("C:/Users/Home/Desktop/Cam-live-streaming-on-MacOS/output.ts");
        
        if (g_file_test(file_path, G_FILE_TEST_EXISTS)) {
            printf("미리 준비된 경로에서 파일을 찾았습니다: %s\n", file_path);
            file_found = true;
        }
    }

    if (!file_found) {
        fprintf(stderr, "output.ts 파일을 찾을 수 없습니다.\n");
        fprintf(stderr, "시도한 경로:\n");
        fprintf(stderr, "1. %s\n", g_build_filename(current_dir, "output.ts", NULL));
        fprintf(stderr, "2. %s\n", "C:/Users/Home/Desktop/Cam-live-streaming-on-MacOS/output.ts");
        g_free(file_path);
        g_free(current_dir);
        return false;
    }

    // 파일 소스 설정
    g_object_set(G_OBJECT(source), "location", file_path, NULL);
    g_free(file_path);
    g_free(current_dir);

    // 파이프라인에 요소 추가
    gst_bin_add_many(GST_BIN(receiver->pipeline), source, demuxer, h264parse, decoder, converter, sink, NULL);

    // 요소 연결
    if (!gst_element_link(source, demuxer)) {
        fprintf(stderr, "Failed to link source to demuxer\n");
        return false;
    }

    // demuxer의 video_%u 패드에 대한 핸들러 설정
    g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), h264parse);

    // 나머지 요소 연결
    if (!gst_element_link_many(h264parse, decoder, converter, sink, NULL)) {
        fprintf(stderr, "Failed to link decoder to sink\n");
        return false;
    }

    // 버스 메시지 핸들러 설정
    GstBus* bus = gst_element_get_bus(receiver->pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::eos", G_CALLBACK(on_eos), receiver);
    g_signal_connect(bus, "message::error", G_CALLBACK(on_error), receiver);
    g_signal_connect(bus, "message::state-changed", G_CALLBACK(on_state_changed), receiver);
    gst_object_unref(bus);

    return true;
}

void on_pad_added(GstElement* element, GstPad* pad, gpointer data) {
    GstElement* h264parse = (GstElement*)data;
    GstPad* sinkpad = gst_element_get_static_pad(h264parse, "sink");
    
    if (!gst_pad_is_linked(sinkpad)) {
        if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
            g_warning("Failed to link demuxer pad to h264parse\n");
        }
    }
    
    gst_object_unref(sinkpad);
}

bool video_receiver_start(VideoReceiver* receiver) {
    if (!receiver || receiver->running) return false;

    // 파이프라인 시작
    GstStateChangeReturn ret = gst_element_set_state(receiver->pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        fprintf(stderr, "Failed to start pipeline\n");
        return false;
    }

    receiver->running = true;
    return true;
}

void video_receiver_stop(VideoReceiver* receiver) {
    if (!receiver || !receiver->running) return;

    gst_element_set_state(receiver->pipeline, GST_STATE_NULL);
    receiver->running = false;
}

bool video_receiver_is_running(const VideoReceiver* receiver) {
    return receiver ? receiver->running : false;
}

void on_eos(GstBus* bus, GstMessage* msg, gpointer data) {
    VideoReceiver* receiver = (VideoReceiver*)data;
    printf("End of stream\n");
    video_receiver_stop(receiver);
}

void on_error(GstBus* bus, GstMessage* msg, gpointer data) {
    VideoReceiver* receiver = (VideoReceiver*)data;
    GError* err;
    gchar* debug_info;
    
    gst_message_parse_error(msg, &err, &debug_info);
    fprintf(stderr, "Error received from element %s: %s\n", 
            GST_OBJECT_NAME(msg->src), err->message);
    fprintf(stderr, "Debugging information: %s\n", 
            debug_info ? debug_info : "none");
    
    g_clear_error(&err);
    g_free(debug_info);
    
    video_receiver_stop(receiver);
}

void on_state_changed(GstBus* bus, GstMessage* msg, gpointer data) {
    VideoReceiver* receiver = (VideoReceiver*)data;
    GstState old_state, new_state, pending_state;
    
    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
    
    // 파이프라인의 상태 변경만 출력
    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(receiver->pipeline)) {
        printf("Pipeline state changed from %s to %s\n",
               gst_element_state_get_name(old_state),
               gst_element_state_get_name(new_state));
    }
} 