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
    #include <libavutil/opt.h> 
}

#include "media/avframe_pool.h"
#include "media/avpacket_pool.h"
#include "thread/safe_queue_ptr.h"
#include "thread/thread.h"

using namespace duck::thread;

namespace duck {
namespace media {

class VideoEncoder : public Thread
{
public:
    VideoEncoder() 
        : Thread("VideoEncoder"), enc_ctx_(nullptr), pkt_queue_(8, "pkt_queue") {}

    ~VideoEncoder() {
        close_encoder();
    }

    int open_encoder(const std::string& codec_name, int width, int height, int framerate = 25, int bit_rate = 400000) {
        int ret;
        const AVCodec *codec;
        /* find the mpeg1video encoder */
        codec = avcodec_find_encoder_by_name(codec_name.c_str());
        if (!codec) {
            LOG(ERROR) << "Codec not found: " << codec_name;
            return -1;
        }

        //Allocate a codec context for the decoder
        enc_ctx_ = avcodec_alloc_context3(codec);
        if (!enc_ctx_) { 
            LOG(ERROR) << "Failed to allocate the video codec context";
            return -1;
        }

        /* put sample parameters */
        enc_ctx_->bit_rate = bit_rate;
        /* resolution must be a multiple of two */
        enc_ctx_->width = width;
        enc_ctx_->height = height;
        /* frames per second */
        enc_ctx_->time_base = (AVRational){1, framerate};
        enc_ctx_->framerate = (AVRational){framerate, 1};

        /* emit one intra frame every ten frames
        * check frame pict_type before passing frame
        * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
        * then gop_size is ignored and the output of encoder
        * will always be I frame irrespective to gop_size
        */
        enc_ctx_->gop_size = 10;
        enc_ctx_->max_b_frames = 1;
        enc_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;

        if (codec->id == AV_CODEC_ID_H264)
            av_opt_set(enc_ctx_->priv_data, "preset", "slow", 0);


        //Init the decoders
        ret = avcodec_open2(enc_ctx_, codec, NULL);
        if (ret < 0) {
            LOG(ERROR) << "Failed to open video codec: " << std::string(av_err2str(ret));
            return -1;
        }

        start();

        return 0;
    }

    void close_encoder() {
        stop();
        avcodec_free_context(&enc_ctx_);
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
            AVFrame *frame = frame_queue_->pop();
            if (frame == nullptr) {
                break;
            }

            ret = encode_frame(frame, &pkt);
            AVFramePool::instance()->put(&frame);
            if (ret < 0) {
                break;
            }

        }
        pkt_queue_.push(nullptr);//flush
    }

    int encode_frame(AVFrame *frame, AVPacket** pkt) {

        int ret;

        // submit the frame to the encoder
        ret = avcodec_send_frame(enc_ctx_, frame);
        if (ret < 0) {
            LOG(ERROR) << "Error sending a frame for encoding: " << std::string(av_err2str(ret));
            return -1;
        }

        while(1)
        {
            //get all the available packets from the encoder 
            ret = avcodec_receive_packet(enc_ctx_, *pkt);
            if (ret < 0) {
                // those two return values are special and mean there is no output
                // packet available, but there were no errors during encoding
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                    break;

                LOG(INFO) << "Error during encoding " << std::string(av_err2str(ret));
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

        return 0; 
    }



    AVPacket* queue_packet() {
        return pkt_queue_.pop();
    }

    void dequeue_packet(AVPacket** pkt) {
        AVPacketPool::instance()->put(pkt);
    }

    void subscribe_frame_queue(SafeQueuePtr<AVFrame>* frame_queue) {
        frame_queue_ = frame_queue;
    }

    SafeQueuePtr<AVPacket>* pkt_queue() {
        return &pkt_queue_;
    }

protected:
    AVCodecContext* enc_ctx_; 
    SafeQueuePtr<AVPacket> pkt_queue_;
    SafeQueuePtr<AVFrame>* frame_queue_;
    
};


}//namespace media
}//namespace duck

