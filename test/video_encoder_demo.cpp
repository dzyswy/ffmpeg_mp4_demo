#include "media/video_encoder.h"
#include "media/yuv_reader.h" 

using namespace duck::media;





int main(int argc, char* argv[])
{
    int ret;
    google::InstallFailureSignalHandler();
    google::InitGoogleLogging(argv[0]);

    if (argc != 7) {
        printf("usage: %s *.yuv width height *.h264 libx264/libx265 (0:INFO, 1:WARNING, 2:ERROR, 3:FATAL)\n", argv[0]);
        printf("usage: %s out.yuv 1920 1080 out.h264 libx264 0\n", argv[0]);
        return -1;
    }

    std::string yuv_name = argv[1];
    int width = atoi(argv[2]);
    int height = atoi(argv[3]);
    int framerate = 25;
    int bit_rate = 12000000;
    std::string out_name = argv[4];
    std::string codec_name = argv[5];
    FLAGS_stderrthreshold = atoi(argv[6]);
    FLAGS_minloglevel = 0;
 
    YuvReader xreader;
    VideoEncoder xencoder;
    xencoder.subscribe_frame_queue(xreader.frame_queue());

    ret = xreader.open_file(yuv_name, width, height);
    if (ret < 0) {
        return -1;
    }

    ret = xencoder.open_encoder(codec_name, width, height, framerate, bit_rate);
    if (ret < 0) {
        return -1;
    }

    FILE *outfp = fopen(out_name.c_str(), "wb");
    if (!outfp) {
        return -1;
    }

    int frame_count = 0;
    while(1)
    {
        AVPacket* pkt = xencoder.queue_packet();
        if (pkt == nullptr) {
            break;
        }

        printf("%d, %x, keyframe=%d, size=%d, pts=%lld, dts=%lld, duration=%lld\n", frame_count, pkt->data[4], pkt->flags,  pkt->size, pkt->pts, pkt->dts, pkt->duration);

        size_t size = fwrite(pkt->data, 1, pkt->size, outfp);
        if (size != pkt->size) {
            printf("fwrite failed-> write:%u, pkt_size:%u\n", size, pkt->size);
        }
        
        xencoder.dequeue_packet(&pkt);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        frame_count++;
        if (frame_count >= 1024) {
            break;
        }
    }

    xencoder.debug();

    fclose(outfp);
    xreader.close_file(); 

    std::cout << "bye!" << std::endl;

    return 0;
}
