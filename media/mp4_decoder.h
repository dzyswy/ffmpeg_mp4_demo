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

#include "media/avframe_pool.h"
#include "thread/safe_queue_ptr.h"
#include "thread/thread.h"

using namespace duck::thread;

namespace duck {
namespace media {


class Mp4Decoder : public Thread
{
public: 
    Mp4Decoder() 
        : Thread("Mp4Decoder"), avfmt_ctx_(nullptr), avfrm_pool_(8, 4, 128), avfrm_queue_(8, "avfrm_queue"), quit_(true), repeat_play_(false) {

    }

    int open_file(const std::string& file_name, bool repeat_play = false) {
        int ret;
        file_name_ = file_name;
 
        AVStream *st;
        const AVCodec *dec = NULL;

        //open input file, and allocate format context
        ret = avformat_open_input(&avfmt_ctx_, file_name_.c_str(), NULL, NULL);
        if (ret < 0) {
            LOG(ERROR) << "Failed to open file: " << file_name_;
            return -1;
        }

        //retrieve stream information
        ret = avformat_find_stream_info(avfmt_ctx_, NULL);
        if (ret < 0) {
            LOG(ERROR) << "Could not find stream information: " << file_name_;
            return -1;
        }

        ret = av_find_best_stream(avfmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (ret < 0) {
            LOG(ERROR) << "Could not find video stream: " << file_name_;
            return -1;
        }

        video_stream_idx_ = ret;
        st = avfmt_ctx_->streams[video_stream_idx_];

        //find decoder for the stream
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            LOG(ERROR) << "Failed to find video codec: " << file_name_;
            return -1;
        }

        //Allocate a codec context for the decoder
        dec_ctx_ = avcodec_alloc_context3(dec);
        if (!dec_ctx_) { 
            LOG(ERROR) << "Failed to allocate the video codec context: " << file_name_;
            return -1;
        }

        //Copy codec parameters from input stream to output codec context
        ret = avcodec_parameters_to_context(dec_ctx_, st->codecpar);
        if (ret < 0) {
            LOG(ERROR) << "Failed to copy video codec parameters to decoder context: " << file_name_;
            return -1;
        }

        //Init the decoders
        ret = avcodec_open2(dec_ctx_, dec, NULL);
        if (ret < 0) {
            LOG(ERROR) << "Failed to open video codec: " << file_name_;
            return -1;
        }

        av_dump_format(avfmt_ctx_, 0, file_name_.c_str(), 0);

        pkt_ = av_packet_alloc();
        if (!pkt_) { 
            LOG(ERROR) << "Could not allocate packet!";
            return -1;
        }

        repeat_play_ = repeat_play;
        //启动解封装和解码线程
        start();
 
        return 0;
    }   

    void close_file() {

        //停止解封装和解码线程
        stop();
   

        avcodec_free_context(&dec_ctx_); 
        avformat_close_input(&avfmt_ctx_);
        av_packet_free(&pkt_);

    }

    virtual void start() override {
        quit_ = false;
        Thread::start();
    }

    void stop() {
        quit_ = true;
        avfrm_queue_.set_quit(true);
        while(is_running());
        join();
    }

    int decode_packet(const AVPacket *pkt, AVFrame** frame) {

        int ret;

        // submit the packet to the decoder
        ret = avcodec_send_packet(dec_ctx_, pkt);
        if (ret < 0) {
            LOG(INFO) << "Error submitting a packet for decoding: " << std::string(av_err2str(ret));
            return ret;
        }

        while(1)
        {
            //get all the available frames from the decoder
            ret = avcodec_receive_frame(dec_ctx_, *frame);
            if (ret < 0) {
                // those two return values are special and mean there is no output
                // frame available, but there were no errors during decoding
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                    break;

                LOG(INFO) << "Error during decoding " << std::string(av_err2str(ret));
                return ret;
            } else {
                ret = avfrm_queue_.push(*frame); 
                if (ret < 0) {
                    return -1;
                }
                *frame = avfrm_pool_.get();
            }
            
        }


        return 0;
    }

    virtual void process() override {

        int ret;

        AVFrame *frame = avfrm_pool_.get();
        while(!quit_)
        { 
            //read frames from the file 
            ret = av_read_frame(avfmt_ctx_, pkt_);
            if (ret < 0) {
                LOG(INFO) << "Failed to av_read_frame: " << std::string(av_err2str(ret)); 
                if (ret == AVERROR_EOF) {
                    if (!repeat_play_) {
                        avfrm_queue_.push(nullptr); 
                        break;
                    } else {
                        av_seek_frame(avfmt_ctx_, video_stream_idx_, 0, AVSEEK_FLAG_BACKWARD);
                        avcodec_flush_buffers(dec_ctx_);
                        ret = av_read_frame(avfmt_ctx_, pkt_);
                        if (ret < 0) {
                            avfrm_queue_.push(nullptr); 
                            break;
                        }
                    }
                    
                } else {
                    continue;
                }
            }

            if (pkt_->stream_index == video_stream_idx_) {
                decode_packet(pkt_, &frame);
            }
            
            av_packet_unref(pkt_);
        }
    }

    AVFrame* queue_frame() {
        AVFrame* frame = avfrm_queue_.pop();
        
        return frame;
    }

    void dequeue_frame(AVFrame** frame) {
        av_frame_unref(*frame);
        avfrm_pool_.put(frame);
    }


protected:
    std::string file_name_;
    AVFormatContext* avfmt_ctx_;
    AVCodecContext* dec_ctx_;
    AVPacket *pkt_; 
    AVFramePool avfrm_pool_;
    SafeQueuePtr<AVFrame> avfrm_queue_;
    int video_stream_idx_;
    bool quit_;
    bool repeat_play_;
};

}//namespace media
}//namespace duck









