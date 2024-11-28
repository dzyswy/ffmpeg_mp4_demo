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
    #include "libavcodec/bsf.h"
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
        : Thread("VideoDemuxer"), fmt_ctx_(nullptr), bsf_ctx_(nullptr), pkt_queue_(8, "avfrm_queue") {}

    ~VideoDemuxer() {
        close_file();
    }

    int open_file(const std::string& file_name) {

        int ret; 
  
        //open input file, and allocate format context
        ret = avformat_open_input(&fmt_ctx_, file_name.c_str(), NULL, NULL);
        if (ret < 0) {
            LOG(ERROR) << "Failed to open file: " << file_name;
            return -1;
        }

        //retrieve stream information
        ret = avformat_find_stream_info(fmt_ctx_, NULL);
        if (ret < 0) {
            LOG(ERROR) << "Could not find stream information";
            return -1;
        }

        ret = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (ret < 0) {
            LOG(ERROR) << "Could not find video stream";
            return -1;
        }

        video_stream_idx_ = ret; 

        av_dump_format(fmt_ctx_, 0, file_name.c_str(), 0);

        if (file_name.find(".mp4") != std::string::npos) {

            /**
            * h264有两种码流格式，一种是avcc，另一种是annexb。
            * mp4支持的是avcc，avcc的NALU前面有NALU的长度。
            * 流媒体传输使用annexb格式，annexb的NALU前面是起始码，有的是4字节的00 00 00 01，有的是3字节的00 00 01。
            * 可以使用过滤器将avcc的码流格式转换为annexb。
            * 
            */
            const AVBitStreamFilter *bsfilter = av_bsf_get_by_name("h264_mp4toannexb");
            av_bsf_alloc(bsfilter, &bsf_ctx_);
            //添加解码器属性
            avcodec_parameters_copy(bsf_ctx_->par_in, fmt_ctx_->streams[video_stream_idx_]->codecpar);
            av_bsf_init(bsf_ctx_);
        }

        start();

        return 0;
    }

    void close_file() {

        stop();
        avformat_close_input(&fmt_ctx_);
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
        while(!pkt_queue_.is_quit())
        {

            ret = av_read_frame(fmt_ctx_, pkt);
            if (ret < 0) {
                LOG(INFO) << "Failed to av_read_frame: " << std::string(av_err2str(ret)); //AVERROR_EOF
                break;
            } else {
                if (pkt->stream_index == video_stream_idx_) {

                    if (bsf_ctx_) {
                        ret = filter_packet(&pkt);
                        if (ret < 0) {
                            break;
                        }
                    } else {
                        ret = pkt_queue_.push(pkt);
                        if (ret < 0) {
                            break;
                        } else {
                            pkt = AVPacketPool::instance()->get();
                        }
                        
                    }
                }
                
            }
        }

        pkt_queue_.push(nullptr); //flush

    }

    int filter_packet(AVPacket **pkt) {

        int ret;

        ret = av_bsf_send_packet(bsf_ctx_, *pkt);
        if (ret < 0) {
            LOG(INFO) << "Failed to av_bsf_send_packet: " << std::string(av_err2str(ret)); 
            return -1;
        } else {
            while(true)
            {
                // After sending each packet, the filter must be completely drained by calling
                // av_bsf_receive_packet() repeatedly until it returns AVERROR(EAGAIN) or AVERROR_EOF.
                ret = av_bsf_receive_packet(bsf_ctx_, *pkt);
                if (ret < 0) {
                
                    if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                        break;

                    LOG(INFO) << "Error during bsf filter: " << std::string(av_err2str(ret));
                    return -1;
                } else {
                    ret = pkt_queue_.push(*pkt); 
                    if (ret < 0) {
                        return -1;
                    } else {
                        *pkt = AVPacketPool::instance()->get();
                    }
                    
                }

            }
        }

        return 0;
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
    AVFormatContext* fmt_ctx_; 
    AVBSFContext *bsf_ctx_;
    SafeQueuePtr<AVPacket> pkt_queue_;
    int video_stream_idx_;

    
};



}//namespace media
}//namespace duck
