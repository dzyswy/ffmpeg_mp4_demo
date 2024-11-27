#include "media/video_demuxer.h"


using namespace duck::media;


int main(int argc, char* argv[])
{
    int ret;
    google::InstallFailureSignalHandler();
    google::InitGoogleLogging(argv[0]);

    if (argc != 3) {
        printf("usage: %s *.mp4 (0:INFO, 1:WARNING, 2:ERROR, 3:FATAL)\n", argv[0]);
        return -1;
    }

    std::string video_name = argv[1];
    FLAGS_stderrthreshold = atoi(argv[2]);
    FLAGS_minloglevel = 0;

    VideoDemuxer xdemuxer;

    ret = xdemuxer.open_file(video_name);
    if (ret < 0) {
        return -1;
    }


    int frame_count = 0;
    while(1)
    {
        AVPacket* pkt = xdemuxer.queue_packet();
        if (pkt == nullptr) {
            break;
        }

        printf("%d, size=%d, pts=%lld, dts=%lld, duration=%lld\n", frame_count, pkt->size, pkt->pts, pkt->dts, pkt->duration);
        
        xdemuxer.dequeue_packet(&pkt);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        frame_count++;
        if (frame_count >= 1024) {
            break;
        }
    }

    std::cout << "wait key..." << std::endl;
    std::getchar(); 

    xdemuxer.close_file();
 

    std::cout << "bye!" << std::endl;

    return 0;
}
































