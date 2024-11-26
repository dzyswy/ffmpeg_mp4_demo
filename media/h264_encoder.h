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
    #include <libavutil/imgutils.h>
}

#include "media/avpacket_pool.h"
#include "thread/safe_queue_ptr.h"
#include "thread/thread.h"

using namespace duck::thread;

namespace duck {
namespace media {

class H264Encoder
{
public:
    H264Encoder() : enc_ctx_(nullptr), avpkt_pool_(8, 4, 128), avpkt_queue_(8, "avpkt_queue") {}

    int open_encoder(int width, int height, int framerate) {
        int ret;
        const AVCodec *codec;

        //find the libx264 encoder
        codec = avcodec_find_encoder_by_name("libx264");
        if (!codec) { 
            LOG(FATAL) << "Codec libx264 not found!";
            return -1;
        }

        enc_ctx_ = avcodec_alloc_context3(codec);
        if (!enc_ctx_) { 
            LOG(FATAL) << "Could not allocate video codec context"; 
            return -1;
        }

        /* put sample parameters */
        enc_ctx_->bit_rate = 400000;
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

        av_opt_set(enc_ctx_->priv_data, "preset", "slow", 0);


        /* open it */
        ret = avcodec_open2(enc_ctx_, codec, NULL);
        if (ret < 0) { 
            LOG(FATAL) << "Could not open codec: " << std::string(av_err2str(ret)); 
            return -1;
        }

        pkt_ = avpkt_pool_.get();

        return 0;
    }

    void close_encoder() {
        avcodec_free_context(&enc_ctx_);
    }

    int encode(AVFrame *frame) {
        int ret;

         
        ret = avcodec_send_frame(enc_ctx_, frame);
        if (ret < 0) { 
            LOG(INFO) << "Error sending a frame for encoding";
            return -1;
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(enc_ctx_, pkt_);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            else if (ret < 0) { 
                LOG(INFO) << "Error during encoding";
                return -1;
            }

            //printf("Write packet %3"PRId64" (size=%5d)\n", pkt->pts, pkt->size);
            avpkt_queue_.push(pkt_);
            pkt_ = avpkt_pool_.get();
        }

        return 0;
    }

    AVPacket* queue_packet() {
        AVPacket* pkt = avpkt_queue_.pop();
        return pkt;
    }

    void dequeue_packet(AVPacket** pkt) {
        av_packet_unref(*pkt);
        avpkt_pool_.put(pkt);
    }

protected:
    AVCodecContext *enc_ctx_;
    AVPacketPool avpkt_pool_;
    AVPacket* pkt_;
    SafeQueuePtr<AVPacket> avpkt_queue_;
};


}//namespace media
}//namespace duck


