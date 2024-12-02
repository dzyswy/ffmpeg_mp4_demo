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


class YuvReader : public Thread
{
public:
    YuvReader() : Thread("YuvReader"), yuvfp_(nullptr), frame_queue_(8, "frame_queue") {}
    ~YuvReader() {
        close_file();
    }

    int open_file(const std::string& file_name, int width, int height, enum AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P) {

        yuvfp_ = fopen(file_name.c_str(), "rb");
        if (!yuvfp_) {
            LOG(ERROR) << "Failed to open yuv file: " << file_name;
            return -1;
        }

        width_ = width;
        height_ = height;
        pix_fmt_ = pix_fmt;
        frame_count_ = 0;

        start();

        return 0;
    }

    void close_file() {

        stop();
        if (yuvfp_) {
            fclose(yuvfp_);
            yuvfp_ = nullptr;
        }
        
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

        
        size_t y_size = width_ * height_;
        size_t u_size = width_ * height_ / 4;
        size_t v_size = width_ * height_ / 4;
        while(!frame_queue_.is_quit())
        {
            AVFrame* frame = alloc_frame();
            if (frame == nullptr) {
                break;
            }

            size_t count_y = fread(frame->data[0], 1, width_ * height_, yuvfp_);
            size_t count_u = fread(frame->data[1], 1, width_ * height_ / 4, yuvfp_);
            size_t count_v = fread(frame->data[2], 1, width_ * height_ / 4, yuvfp_);
            if ((count_y != y_size) || (count_u != u_size) || (count_v != v_size)) {
                AVFramePool::instance()->put(&frame);
                break;
            }
            frame->pts = frame_count_;
            frame_queue_.push(frame);
            frame_count_++;
            
        }
        frame_queue_.push(nullptr);//flush
    }

    AVFrame* alloc_frame() {
        AVFrame* frame = AVFramePool::instance()->get();

        frame->format = pix_fmt_;
        frame->width  = width_;
        frame->height = height_;
        int ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) { 
            LOG(ERROR) << "Could not allocate the video frame data";
            AVFramePool::instance()->put(&frame);
            return nullptr;
        }
        return frame;
    }

    AVFrame* queue_frame() {
        AVFrame* frame = frame_queue_.pop();
        
        return frame;
    }

    void dequeue_frame(AVFrame** frame) { 
        AVFramePool::instance()->put(frame);
    }


    SafeQueuePtr<AVFrame>* frame_queue() {
        return &frame_queue_;
    }

protected:
    FILE *yuvfp_;
    int width_;
    int height_;
    enum AVPixelFormat pix_fmt_;

    SafeQueuePtr<AVFrame> frame_queue_;
    int64_t frame_count_;
};


}//namespace media
}//namespace duck


