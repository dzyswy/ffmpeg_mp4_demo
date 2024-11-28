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
#include "media/avpacket_pool.h"
#include "thread/safe_queue_ptr.h"
#include "thread/thread.h"

using namespace duck::thread;

namespace duck {
namespace media {


class VideoDecoder : public Thread
{
public:
    VideoDecoder() 
        : Thread("VideoDecoder"), frame_queue_(8, "frame_queue") {}

    ~VideoDecoder() {
        close_decoder();
    }

    int open_decoder(const std::string& codec_name) {
        int ret;
        const AVCodec *codec;
        codec = avcodec_find_decoder_by_name(codec_name.c_str());
        //codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec) { 
            LOG(ERROR) << "Codec not found: " << codec_name;
            return -1; 
        }

        //Allocate a codec context for the decoder
        dec_ctx_ = avcodec_alloc_context3(codec);
        if (!dec_ctx_) { 
            LOG(ERROR) << "Failed to allocate the video codec context";
            return -1;
        }

        //Init the decoders
        ret = avcodec_open2(dec_ctx_, codec, NULL);
        if (ret < 0) {
            LOG(ERROR) << "Failed to open video codec";
            return -1;
        }

        start();

        return 0;
    }

    void close_decoder() {

        stop();
        avcodec_free_context(&dec_ctx_); 
    }

    void start() override {

        Thread::start();
    }

    void stop() {
        frame_queue_.set_quit(true);
        while(is_running());
        join();
    }

    void process() override {

        int ret;

        AVFrame *frame = AVFramePool::instance()->get();
        while(!frame_queue_.is_quit())
        {
            AVPacket *pkt = pkt_queue_->pop();
            if (pkt == nullptr) {
                break;
            }

            ret = decode_packet(pkt, &frame);
            AVPacketPool::instance()->put(&pkt);
            if (ret < 0) {
                break;
            }


        }
        frame_queue_.push(nullptr); //flush
    }

    int decode_packet(const AVPacket *pkt, AVFrame** frame) {

        int ret;

        // submit the packet to the decoder
        ret = avcodec_send_packet(dec_ctx_, pkt);
        if (ret < 0) {
            LOG(ERROR) << "Error submitting a packet for decoding: " << std::string(av_err2str(ret));
            return -1;
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
                return -1;
            } else {
                ret = frame_queue_.push(*frame); 
                if (ret < 0) {
                    return -1;
                } else {
                    *frame = AVFramePool::instance()->get();
                }
            }
        }

        return 0;
    }

    AVFrame* queue_frame() {
        AVFrame* frame = frame_queue_.pop();
        
        return frame;
    }

    void dequeue_frame(AVFrame** frame) { 
        AVFramePool::instance()->put(frame);
    }

    void subscribe_pkt_queue(SafeQueuePtr<AVPacket>* pkt_queue) {
        pkt_queue_ = pkt_queue;
    }

    SafeQueuePtr<AVFrame>* frame_queue() {
        return &frame_queue_;
    }

protected:
    AVCodecContext* dec_ctx_; 
    SafeQueuePtr<AVFrame> frame_queue_;
    SafeQueuePtr<AVPacket>* pkt_queue_;
};


}//namespace media
}//namespace duck
