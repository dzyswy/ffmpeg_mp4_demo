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
        enc_ctx_->max_b_frames = 0;
        enc_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;

        if (codec->id == AV_CODEC_ID_H264)
            av_opt_set(enc_ctx_->priv_data, "preset", "slow", 0);

        //enc_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

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
        int ret = avcodec_parameters_from_context(codecpar, enc_ctx_);
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

        for (int i = 0; i < codecpar->extradata_size; i++) {
            printf("%02x ", codecpar->extradata[i]);
            if ((i % 16) == 15) {
                printf("\n");
            }
        }
        printf("\n");


        avcodec_parameters_free(&codecpar);
        //debug
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

