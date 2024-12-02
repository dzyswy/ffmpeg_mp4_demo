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
    #include <libavformat/avio.h>
    #include <libavutil/opt.h> 
}

#include "media/avframe_pool.h"
#include "media/avpacket_pool.h"
#include "thread/safe_queue_ptr.h"
#include "thread/thread.h"

using namespace duck::thread;

namespace duck {
namespace media {

class VideoMuxer : public Thread
{
public:
    VideoMuxer() 
        : Thread("VideoMuxer"), fmt_ctx_(nullptr) {}
    ~VideoMuxer() {

    }

    int open_file(const std::string& file_name, int width, int height, int framerate, enum AVCodecID codec_id) {

        int ret; 
        AVDictionary *opt = NULL;

        ret = avformat_alloc_output_context2(&fmt_ctx_, NULL, NULL, file_name.c_str());
        if (ret < 0) {
            LOG(ERROR) << "Failed to avformat_alloc_output_context2!";
            return -1;
        }
 
        video_st_ = avformat_new_stream(fmt_ctx_, NULL);
        if (!video_st_) {
            LOG(ERROR) << "Could not allocate stream";
            return -1;
        }
        video_st_->id = fmt_ctx_->nb_streams-1;
        video_st_->time_base = (AVRational){ 1, framerate};
        time_base_ = (AVRational){ 1, framerate};

        video_st_->codecpar = avcodec_parameters_alloc();
        video_st_->codecpar->width = width;
        video_st_->codecpar->height = height;
        video_st_->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
        video_st_->codecpar->codec_id = codec_id;//AV_CODEC_ID_H264
        video_st_->codecpar->format = AV_PIX_FMT_YUV420P;
        //video_st_->codecpar->bit_rate = ;
        
        av_dump_format(fmt_ctx_, 0, file_name.c_str(), 1);

        ret = avio_open(&fmt_ctx_->pb, file_name.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) { 
            LOG(ERROR) << "Could not open file: " << file_name << ", " << std::string(av_err2str(ret));
            return -1;
        }

        /* Write the stream header, if any. */
        ret = avformat_write_header(fmt_ctx_, &opt);
        if (ret < 0) {
            fprintf(stderr, "Error occurred when opening output file: %s\n",
                    av_err2str(ret));
            return 1;
        }


        printf("time_base: AVCodecContext=(%d %d), AVStream=(%d %d)\n", time_base_.num, time_base_.den, video_st_->time_base.num, video_st_->time_base.den);


        start();

        return 0;
    }

    void close_file() {

        stop();

        av_write_trailer(fmt_ctx_);
        avio_closep(&fmt_ctx_->pb);
        avformat_free_context(fmt_ctx_);
    }

    void start() override {

        Thread::start();
    }

    void stop() {
        //pkt_queue_.set_quit(true);
        while(is_running());
        join();
    }

    void process() override {

        int ret;
        while(true)
        {
            AVPacket* pkt = pkt_queue_->pop();
            if (pkt == nullptr) {
                break;
            }

            printf("pkt: pts=%lld, dts=%lld, duration=%lld\n", pkt->pts, pkt->dts, pkt->duration);
            
            /* rescale output packet timestamp values from codec to stream timebase */
            av_packet_rescale_ts(pkt, time_base_, video_st_->time_base);
            pkt->stream_index = video_st_->index;

            log_packet(fmt_ctx_, pkt);
            //ret = av_write_frame(fmt_ctx_, pkt);
            ret = av_interleaved_write_frame(fmt_ctx_, pkt);
            if (ret < 0) {
                LOG(ERROR) << "Failed to av_write_frame";
            }
            AVPacketPool::instance()->put(&pkt);
        }

    }

    static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
    {
        AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

        printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
            av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
            av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
            av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
            pkt->stream_index);
    }

    void subscribe_pkt_queue(SafeQueuePtr<AVPacket>* pkt_queue) {
        pkt_queue_ = pkt_queue;
    }


protected:
    AVFormatContext* fmt_ctx_; 
    AVStream* video_st_;
    SafeQueuePtr<AVPacket>* pkt_queue_;
    AVRational time_base_;
};


}//namespace media
}//namespace duck


