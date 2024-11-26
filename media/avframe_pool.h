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
    AVFramePool(int init_num, int inc_num, int max_num) : pool_(init_num, inc_num, max_num, &allocator_) {}

    AVFrame* get() {
        return pool_.get();
    }

    void put(AVFrame** p) {
        pool_.put(p);
    }

protected:
    AVFrameAllocator allocator_;
    ObjectPool<AVFrame> pool_;
};




}//namespace media
}//namespace duck
