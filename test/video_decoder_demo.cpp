#include "media/video_demuxer.h"
#include "media/video_decoder.h"  

using namespace duck::media;


static uint8_t *video_dst_data[4] = {NULL};
static int      video_dst_linesize[4];
static int video_dst_bufsize = 0;




int main(int argc, char* argv[])
{
    int ret;
    google::InstallFailureSignalHandler();
    google::InitGoogleLogging(argv[0]);

    if (argc != 5) {
        printf("usage: %s *.h264 h264/hevc *.yuv (0:INFO, 1:WARNING, 2:ERROR, 3:FATAL)\n", argv[0]);
        return -1;
    }

    std::string video_name = argv[1];
    std::string codec_name = argv[2];
    std::string out_name = argv[3];
    FLAGS_stderrthreshold = atoi(argv[4]);
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

    FILE *outfp = fopen(out_name.c_str(), "wb");
    if (!outfp) {
        return -1;
    }

 
    int frame_count = 0;
    while(1)
    {
        AVFrame* frame = xdecoder.queue_frame();
        if (frame == nullptr) {
            break;
        }

        if (video_dst_bufsize == 0) {
            video_dst_bufsize = av_image_alloc(video_dst_data, video_dst_linesize,
                             frame->width, frame->height, AV_PIX_FMT_YUV420P, 1);
            if (video_dst_bufsize < 0) {
                printf("Failed to av_image_alloc!\n");
                return -1;
            }
        }

        /* copy decoded frame to destination buffer:
        * this is required since rawvideo expects non aligned data */
        av_image_copy(video_dst_data, video_dst_linesize,
                    (const uint8_t **)(frame->data), frame->linesize,
                    AV_PIX_FMT_YUV420P, frame->width, frame->height);

        /* write to rawvideo file */
        fwrite(video_dst_data[0], 1, video_dst_bufsize, outfp);

        cv::Mat img = cv::Mat(frame->height, frame->width, CV_8U, frame->data[0]);
        cv::imshow("img", img);

        cv::waitKey(10);

        printf("%d, width=%d, height=%d, pts=%lld, pkt_dts=%lld\n", frame_count, frame->width, frame->height, frame->pts, frame->pkt_dts);
        xdecoder.dequeue_frame(&frame);

        //std::this_thread::sleep_for(std::chrono::milliseconds(25));
        frame_count++;
        if (frame_count >= 1024) {
            break;
        }
    }

    xdecoder.debug();

    fclose(outfp);
    cv::destroyAllWindows();

    std::cout << "wait key..." << std::endl;
    std::getchar(); 

    xdecoder.close_decoder();

    xdemuxer.close_file();
 

    std::cout << "bye!" << std::endl;

    return 0;
}
































