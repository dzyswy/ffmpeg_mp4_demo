#include "media/video_demuxer.h"
#include "media/video_decoder.h"

using namespace duck::media;


int main(int argc, char* argv[])
{
    int ret;
    google::InstallFailureSignalHandler();
    google::InitGoogleLogging(argv[0]);

    if (argc != 4) {
        printf("usage: %s *.h264 h264/hevc (0:INFO, 1:WARNING, 2:ERROR, 3:FATAL)\n", argv[0]);
        return -1;
    }

    std::string video_name = argv[1];
    std::string codec_name = argv[2];
    FLAGS_stderrthreshold = atoi(argv[3]);
    FLAGS_minloglevel = 0;

    VideoDemuxer xdemuxer;
    VideoDecoder xdecoder;
    xdecoder.subscribe_pkt_queue(xdemuxer.pkt_queue());

    ret = xdemuxer.open_file(video_name);
    if (ret < 0) {
        return -1;
    }

    ret = xdecoder.open_decoder(codec_name);
    if (ret < 0) {
        return -1;
    }

    
    int frame_count = 0;
    while(1)
    {
        AVFrame* frame = xdecoder.queue_frame();
        if (frame == nullptr) {
            break;
        }
        printf("%d, width=%d, height=%d, pts=%lld, pkt_dts=%lld\n", frame_count, frame->width, frame->height, frame->pts, frame->pkt_dts);
        xdecoder.dequeue_frame(&frame);

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        frame_count++;
        if (frame_count >= 1024) {
            break;
        }
    }

    std::cout << "wait key..." << std::endl;
    std::getchar(); 

    xdecoder.close_decoder();

    xdemuxer.close_file();
 

    std::cout << "bye!" << std::endl;

    return 0;
}
































