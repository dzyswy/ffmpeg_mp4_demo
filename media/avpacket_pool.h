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


class AVPacketAllocator : public ObjectAllocator
{
public:
    void* allocate() override {

        AVPacket* pkt = av_packet_alloc();
 
        LOG(INFO) << "allocate AVPacket object: " << ((void*)pkt);
        return (void*)pkt;
    }

    void deallocate(void** p) override {
        LOG(INFO) << "deallocate AVPacket object: " << (*(void**)p);
        AVPacket** pkt = (AVPacket**)p;
        av_packet_free(pkt);  
    }
};

class AVPacketPool
{
public:
    AVPacketPool(int init_num, int inc_num, int max_num) : pool_(init_num, inc_num, max_num, &allocator_) {}

    AVPacket* get() {
        return pool_.get();
    }

    void put(AVPacket** p) {
        pool_.put(p);
    }

protected:
    AVPacketAllocator allocator_;
    ObjectPool<AVPacket> pool_;
};



}//namespace media
}//namespace duck