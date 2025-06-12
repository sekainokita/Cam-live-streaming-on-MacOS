#pragma once

#include <gst/gst.h>
#include <string>
#include <memory>

class TSPlayer {
public:
    TSPlayer();
    ~TSPlayer();

    // 초기화 및 시작/중지
    bool initialize();
    bool loadFile(const std::string& filename);
    bool start();
    void stop();

    // 재생 제어
    void pause();
    void resume();
    void seek(gint64 position);
    void setVolume(double volume);
    void setPlaybackRate(double rate);

    // 상태 정보
    bool isPlaying() const;
    gint64 getDuration() const;
    gint64 getPosition() const;
    double getVolume() const;
    double getPlaybackRate() const;

private:
    // GStreamer 파이프라인 구성요소
    GstElement* pipeline_;
    GstElement* source_;
    GstElement* demuxer_;
    GstElement* decoder_;
    GstElement* converter_;
    GstElement* sink_;
    GstElement* queue_;

    // 재생 상태
    bool is_playing_;
    double volume_;
    double playback_rate_;
    gint64 duration_;

    // 내부 메서드
    bool createPipeline();
    void cleanupPipeline();
    static void onBusMessage(GstBus* bus, GstMessage* msg, gpointer data);
    static void onPadAdded(GstElement* element, GstPad* pad, gpointer data);
    void updateDuration();
}; 