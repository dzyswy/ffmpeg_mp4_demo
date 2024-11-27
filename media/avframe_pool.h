#pragma once

#include "object/object_pool.h"

extern "C" {
    #include <libavutil/imgutils.h>
    #include <libavutil/samplefmt.h>
    #include <libavutil/timestamp.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

using namespace duck::object;

namespace duck {
namespace media {


class AVFrameAllocator : public ObjectAllocator
{
public:
    void* allocate() override {

        AVFrame* frame = av_frame_alloc();
 
        LOG(INFO) << "allocate AVFrame object: " << ((void*)frame);
        return (void*)frame;
    }

    void deallocate(void** p) override {
        LOG(INFO) << "deallocate AVFrame object: " << (*(void**)p);
        AVFrame** frame = (AVFrame**)p;
        av_frame_free(frame);  
    }
};

class AVFramePool
{
public: 
    static AVFramePool* instance() {
        static AVFramePool instance;
        return &instance;
    }


    AVFrame* get() {
        return pool_.get();
    }

    void put(AVFrame** p) {
        av_frame_unref(*p);
        pool_.put(p);
    }

protected:
    AVFrameAllocator allocator_;
    ObjectPool<AVFrame> pool_;

private:
    AVFramePool() : pool_(8, 4, 128, &allocator_) {}
    ~AVFramePool() {}
    AVFramePool(const AVFramePool&) = delete;
    AVFramePool & operator = (const AVFramePool&) = delete;
};




}//namespace media
}//namespace duck

