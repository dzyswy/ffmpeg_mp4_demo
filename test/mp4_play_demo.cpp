#include "media/mp4_decoder.h"


using namespace duck::media;


int main(int argc, char* argv[])
{
    google::InstallFailureSignalHandler();
    google::InitGoogleLogging(argv[0]);

    if (argc != 3) {
        printf("usage: %s *.mp4 (0:INFO, 1:WARNING, 2:ERROR, 3:FATAL)\n", argv[0]);
        return -1;
    }

    std::string mp4_name = argv[1];
    FLAGS_stderrthreshold = atoi(argv[2]);
    FLAGS_minloglevel = 0;

    Mp4Decoder mp4x;
    mp4x.open_file(mp4_name, true);

    int frame_count = 0;
    while(1)
    {
        AVFrame* frame = mp4x.queue_frame();
        if (frame == nullptr) {
            break;
        }
        printf("%d, width=%d, height=%d, pts=%lld, pkt_dts=%lld\n", frame_count, frame->width, frame->height, frame->pts, frame->pkt_dts);
        mp4x.dequeue_frame(&frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        frame_count++;
        if (frame_count >= 102400) {
            break;
        }
    }

    std::cout << "wait key..." << std::endl;
    std::getchar(); 

    mp4x.close_file();
 

    std::cout << "bye!" << std::endl;

    return 0;
}
































