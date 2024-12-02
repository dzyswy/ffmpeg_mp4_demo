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

    void debug() {
        //debug
        AVCodecParameters *codecpar = avcodec_parameters_alloc();
        LOG(INFO) << "codec_type: \t" << (int)codecpar->codec_type;
        LOG(INFO) << "codec_id: \t" << (int)codecpar->codec_id;
        LOG(INFO) << "codec_tag: \t" << codecpar->codec_tag;
        LOG(INFO) << "extradata_size: \t" << (int)codecpar->extradata_size;
        LOG(INFO) << "format: \t" << (int)codecpar->format;
        LOG(INFO) << "bit_rate: \t" << codecpar->bit_rate;
        LOG(INFO) << "bits_per_coded_sample: \t" << (int)codecpar->bits_per_coded_sample;
        LOG(INFO) << "bits_per_raw_sample: \t" << (int)codecpar->bits_per_raw_sample;
        LOG(INFO) << "profile: \t" << (int)codecpar->profile;
        LOG(INFO) << "level: \t" << (int)codecpar->level;
        LOG(INFO) << "width: \t" << (int)codecpar->width;
        LOG(INFO) << "height: \t" << (int)codecpar->height;


        LOG(INFO) << "sample_aspect_ratio: \t" << (int)codecpar->sample_aspect_ratio.num << ", " << codecpar->sample_aspect_ratio.den ;
        LOG(INFO) << "field_order: \t" << (int)codecpar->field_order;
        LOG(INFO) << "color_range: \t" << (int)codecpar->color_range;
        LOG(INFO) << "color_primaries: \t" << (int)codecpar->color_primaries;
        LOG(INFO) << "color_trc: \t" << (int)codecpar->color_trc;
        LOG(INFO) << "color_space: \t" << (int)codecpar->color_space;
        LOG(INFO) << "chroma_location: \t" << (int)codecpar->chroma_location;
        LOG(INFO) << "video_delay: \t" << (int)codecpar->video_delay;
        int ret = avcodec_parameters_from_context(codecpar, dec_ctx_);
        if (ret < 0) {
            LOG(ERROR) << "Failed to avcodec_parameters_from_context";
            return;
        }
        LOG(INFO) << "\n\n";

        LOG(INFO) << "codec_type: \t" << (int)codecpar->codec_type;
        LOG(INFO) << "codec_id: \t" << (int)codecpar->codec_id;
        LOG(INFO) << "codec_tag: \t" << codecpar->codec_tag;
        LOG(INFO) << "extradata_size: \t" << (int)codecpar->extradata_size;
        LOG(INFO) << "format: \t" << (int)codecpar->format;
        LOG(INFO) << "bit_rate: \t" << codecpar->bit_rate;
        LOG(INFO) << "bits_per_coded_sample: \t" << (int)codecpar->bits_per_coded_sample;
        LOG(INFO) << "bits_per_raw_sample: \t" << (int)codecpar->bits_per_raw_sample;
        LOG(INFO) << "profile: \t" << (int)codecpar->profile;
        LOG(INFO) << "level: \t" << (int)codecpar->level;
        LOG(INFO) << "width: \t" << (int)codecpar->width;
        LOG(INFO) << "height: \t" << (int)codecpar->height;


        LOG(INFO) << "sample_aspect_ratio: \t" << (int)codecpar->sample_aspect_ratio.num << ", " << codecpar->sample_aspect_ratio.den ;
        LOG(INFO) << "field_order: \t" << (int)codecpar->field_order;
        LOG(INFO) << "color_range: \t" << (int)codecpar->color_range;
        LOG(INFO) << "color_primaries: \t" << (int)codecpar->color_primaries;
        LOG(INFO) << "color_trc: \t" << (int)codecpar->color_trc;
        LOG(INFO) << "color_space: \t" << (int)codecpar->color_space;
        LOG(INFO) << "chroma_location: \t" << (int)codecpar->chroma_location;
        LOG(INFO) << "video_delay: \t" << (int)codecpar->video_delay;


        avcodec_parameters_free(&codecpar);
        //debug
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
