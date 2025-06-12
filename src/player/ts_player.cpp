#include "ts_player.hpp"
#include <iostream>
#include <gst/gst.h>

TSPlayer::TSPlayer()
    : pipeline_(nullptr)
    , source_(nullptr)
    , demuxer_(nullptr)
    , decoder_(nullptr)
    , converter_(nullptr)
    , sink_(nullptr)
    , queue_(nullptr)
    , is_playing_(false)
    , volume_(1.0)
    , playback_rate_(1.0)
    , duration_(0)
{
}

TSPlayer::~TSPlayer()
{
    stop();
    cleanupPipeline();
}

bool TSPlayer::initialize()
{
    // GStreamer 초기화
    gst_init(nullptr, nullptr);

    // 파이프라인 생성
    return createPipeline();
}

bool TSPlayer::createPipeline()
{
    // 파이프라인 생성
    pipeline_ = gst_pipeline_new("ts-player-pipeline");
    if (!pipeline_) {
        std::cerr << "Failed to create pipeline" << std::endl;
        return false;
    }

    // 요소 생성
    source_ = gst_element_factory_make("filesrc", "source");
    queue_ = gst_element_factory_make("queue", "queue");
    demuxer_ = gst_element_factory_make("tsdemux", "demuxer");
    decoder_ = gst_element_factory_make("h264dec", "decoder");
    converter_ = gst_element_factory_make("videoconvert", "converter");
    sink_ = gst_element_factory_make("d3dvideosink", "sink");

    if (!source_ || !queue_ || !demuxer_ || !decoder_ || !converter_ || !sink_) {
        std::cerr << "Failed to create elements" << std::endl;
        return false;
    }

    // 파이프라인에 요소 추가
    gst_bin_add_many(GST_BIN(pipeline_), source_, queue_, demuxer_, decoder_, converter_, sink_, nullptr);

    // 요소 연결
    if (!gst_element_link(source_, queue_) ||
        !gst_element_link(queue_, demuxer_)) {
        std::cerr << "Failed to link elements" << std::endl;
        return false;
    }

    // demuxer의 동적 패드 연결을 위한 시그널 설정
    g_signal_connect(demuxer_, "pad-added", G_CALLBACK(onPadAdded), this);

    // 버스 메시지 핸들러 설정
    GstBus* bus = gst_element_get_bus(pipeline_);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message", G_CALLBACK(onBusMessage), this);
    gst_object_unref(bus);

    return true;
}

void TSPlayer::cleanupPipeline()
{
    if (pipeline_) {
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
}

bool TSPlayer::loadFile(const std::string& filename)
{
    if (!source_) {
        return false;
    }

    g_object_set(G_OBJECT(source_), "location", filename.c_str(), nullptr);
    updateDuration();
    return true;
}

bool TSPlayer::start()
{
    if (!pipeline_) {
        return false;
    }

    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    is_playing_ = (ret != GST_STATE_CHANGE_FAILURE);
    return is_playing_;
}

void TSPlayer::stop()
{
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        is_playing_ = false;
    }
}

void TSPlayer::pause()
{
    if (pipeline_ && is_playing_) {
        gst_element_set_state(pipeline_, GST_STATE_PAUSED);
        is_playing_ = false;
    }
}

void TSPlayer::resume()
{
    if (pipeline_ && !is_playing_) {
        gst_element_set_state(pipeline_, GST_STATE_PLAYING);
        is_playing_ = true;
    }
}

void TSPlayer::seek(gint64 position)
{
    if (pipeline_) {
        gst_element_seek(pipeline_,
            playback_rate_,
            GST_FORMAT_TIME,
            static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
            GST_SEEK_TYPE_SET,
            position,
            GST_SEEK_TYPE_NONE,
            GST_CLOCK_TIME_NONE);
    }
}

void TSPlayer::setVolume(double volume)
{
    volume_ = std::max(0.0, std::min(1.0, volume));
    if (sink_) {
        g_object_set(G_OBJECT(sink_), "volume", volume_, nullptr);
    }
}

void TSPlayer::setPlaybackRate(double rate)
{
    playback_rate_ = rate;
    // 재생 속도 변경은 seek를 통해 적용
    gint64 position = getPosition();
    seek(position);
}

bool TSPlayer::isPlaying() const
{
    return is_playing_;
}

gint64 TSPlayer::getDuration() const
{
    return duration_;
}

gint64 TSPlayer::getPosition() const
{
    gint64 position = 0;
    if (pipeline_) {
        gst_element_query_position(pipeline_, GST_FORMAT_TIME, &position);
    }
    return position;
}

double TSPlayer::getVolume() const
{
    return volume_;
}

double TSPlayer::getPlaybackRate() const
{
    return playback_rate_;
}

void TSPlayer::updateDuration()
{
    duration_ = 0;
    if (pipeline_) {
        gst_element_query_duration(pipeline_, GST_FORMAT_TIME, &duration_);
    }
}

void TSPlayer::onBusMessage(GstBus* bus, GstMessage* msg, gpointer data)
{
    TSPlayer* player = static_cast<TSPlayer*>(data);

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError* err;
            gchar* debug_info;
            gst_message_parse_error(msg, &err, &debug_info);
            std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src) << ": " << err->message << std::endl;
            std::cerr << "Debugging information: " << (debug_info ? debug_info : "none") << std::endl;
            g_clear_error(&err);
            g_free(debug_info);
            break;
        }
        case GST_MESSAGE_EOS:
            std::cout << "End-Of-Stream reached" << std::endl;
            player->stop();
            break;
        case GST_MESSAGE_DURATION_CHANGED:
            player->updateDuration();
            break;
        case GST_MESSAGE_STATE_CHANGED: {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(player->pipeline_)) {
                std::cout << "Pipeline state changed from " << gst_element_state_get_name(old_state)
                         << " to " << gst_element_state_get_name(new_state) << std::endl;
            }
            break;
        }
        default:
            break;
    }
}

void TSPlayer::onPadAdded(GstElement* element, GstPad* pad, gpointer data)
{
    TSPlayer* player = static_cast<TSPlayer*>(data);
    GstPad* sink_pad = gst_element_get_static_pad(player->decoder_, "sink");
    
    if (!sink_pad) {
        std::cerr << "Failed to get decoder sink pad" << std::endl;
        return;
    }

    if (gst_pad_is_linked(sink_pad)) {
        gst_object_unref(sink_pad);
        return;
    }

    GstPadLinkReturn ret = gst_pad_link(pad, sink_pad);
    if (ret != GST_PAD_LINK_OK) {
        std::cerr << "Failed to link pads" << std::endl;
    }

    gst_object_unref(sink_pad);
} 