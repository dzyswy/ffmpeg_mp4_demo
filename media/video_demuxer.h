#pragma once

#include <string>
#include <iostream>
#include <iomanip> 
#include <map>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <functional>
#include <glog/logging.h>
#include <opencv2/opencv.hpp>

extern "C" {
    #include <libavutil/imgutils.h>
    #include <libavutil/samplefmt.h>
    #include <libavutil/timestamp.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

#include "media/avpacket_pool.h"
#include "thread/safe_queue_ptr.h"
#include "thread/thread.h"

using namespace duck::thread;

namespace duck {
namespace media {


class VideoDemuxer : public Thread
{
public:
    VideoDemuxer() 
        : Thread("VideoDemuxer"), avfmt_ctx_(nullptr), pkt_queue_(8, "avfrm_queue") {}

    ~VideoDemuxer() {
        close_file();
    }

    int open_file(const std::string& file_name) {

        int ret; 
  
        //open input file, and allocate format context
        ret = avformat_open_input(&avfmt_ctx_, file_name.c_str(), NULL, NULL);
        if (ret < 0) {
            LOG(ERROR) << "Failed to open file: " << file_name;
            return -1;
        }

        //retrieve stream information
        ret = avformat_find_stream_info(avfmt_ctx_, NULL);
        if (ret < 0) {
            LOG(ERROR) << "Could not find stream information";
            return -1;
        }

        ret = av_find_best_stream(avfmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (ret < 0) {
            LOG(ERROR) << "Could not find video stream";
            return -1;
        }

        video_stream_idx_ = ret; 

        av_dump_format(avfmt_ctx_, 0, file_name.c_str(), 0);

        start();

        return 0;
    }

    void close_file() {

        stop();
        avformat_close_input(&avfmt_ctx_);
    }

    void start() override {

        Thread::start();
    }

    void stop() {
        pkt_queue_.set_quit(true);
        while(is_running());
        join();
    }

    void process() override {

        int ret;
        AVPacket* pkt = AVPacketPool::instance()->get();
        while(true)
        {
            if (pkt_queue_.is_quit()) {
                break;
            }

            ret = av_read_frame(avfmt_ctx_, pkt);
            if (ret < 0) {
                LOG(INFO) << "Failed to av_read_frame: " << std::string(av_err2str(ret)); 
                if (ret == AVERROR_EOF) {
                    pkt_queue_.push(nullptr); //flush
                } 
                break;
            } else {
                if (pkt->stream_index == video_stream_idx_) {
                    pkt_queue_.push(pkt);
                    pkt = AVPacketPool::instance()->get();
                }
                
            }
        }

    }

    AVPacket* queue_packet() {
        return pkt_queue_.pop();
    }

    void dequeue_packet(AVPacket** pkt) {
        AVPacketPool::instance()->put(pkt);
    }

    SafeQueuePtr<AVPacket>* pkt_queue() {
        return &pkt_queue_;
    }

protected: 
    AVFormatContext* avfmt_ctx_; 
    SafeQueuePtr<AVPacket> pkt_queue_;
    int video_stream_idx_;

    
};



}//namespace media
}//namespace duck
