#include "media/video_muxer.h"
#include "media/video_demuxer.h"

using namespace duck::media;


int main(int argc, char* argv[])
{
    int ret;
    google::InstallFailureSignalHandler();
    google::InitGoogleLogging(argv[0]);

    if (argc != 6) {
        printf("usage: %s *.h264 *.mp4 width height (0:INFO, 1:WARNING, 2:ERROR, 3:FATAL)\n", argv[0]);
        printf("usage: %s out.h264 out.mp4 1920 1080 0\n", argv[0]);
        return -1;
    }

    std::string video_name = argv[1];
    std::string out_name = argv[2];
    int width = atoi(argv[3]);
    int height = atoi(argv[4]);
    FLAGS_stderrthreshold = atoi(argv[5]);
    FLAGS_minloglevel = 0;

    VideoDemuxer xdemuxer;
    VideoMuxer xmuxer;
    xmuxer.subscribe_pkt_queue(xdemuxer.pkt_queue());

    ret = xdemuxer.open_file(video_name);
    if (ret < 0) {
        return -1;
    }

    enum AVCodecID codec_id = AV_CODEC_ID_H264;
    if (video_name.find(".h265") != std::string::npos) {
        codec_id = AV_CODEC_ID_H265;
    }

    ret = xmuxer.open_file(out_name, width, height, 30, codec_id);
    if (ret < 0) {
        return -1;
    }

    xmuxer.close_file();
    xdemuxer.close_file();

    std::cout << "bye!" << std::endl;

    return 0;
}