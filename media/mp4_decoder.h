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

namespace duck {
namespace media {


class Mp4Decoder
{
public: 
    Mp4Decoder() : avfmt_ctx_(nullptr) {

    }

    int open_file(const std::string& file_name) {
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

        return 0;
    }   

    void close_file() {

    }

    void process() {

        // while(true)
        // { 
        //     //read frames from the file 
        //     av_read_frame(avfmt_ctx_, pkt);
        //     // submit the packet to the decoder
        //     ret = avcodec_send_packet(dec, pkt);
        //     while(true)
        //     {
        //         AVFrame *frame = frame_pool_.get();
        //         ret = avcodec_receive_frame(dec, frame);
        //         frame_queue_.push(frame);
        //     }
        // }
    }


protected:
    std::string file_name_;
    AVFormatContext* avfmt_ctx_;
    AVCodecContext* dec_ctx_;
    int video_stream_idx_;
};

}//namespace media
}//namespace duck









