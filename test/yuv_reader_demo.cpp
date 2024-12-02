#include "media/yuv_reader.h" 

using namespace duck::media;







int main(int argc, char* argv[])
{
    int ret;
    google::InstallFailureSignalHandler();
    google::InitGoogleLogging(argv[0]);

    if (argc != 5) {
        printf("usage: %s *.yuv width height (0:INFO, 1:WARNING, 2:ERROR, 3:FATAL)\n", argv[0]);
        return -1;
    }

    std::string yuv_name = argv[1];
    int width = atoi(argv[2]);
    int height = atoi(argv[3]); 
    FLAGS_stderrthreshold = atoi(argv[4]);
    FLAGS_minloglevel = 0;

    YuvReader xreader;

    ret = xreader.open_file(yuv_name, width, height);
    if (ret < 0) {
        return -1;
    }

    int frame_count = 0;
    while(1)
    {
        AVFrame* frame = xreader.queue_frame();
        if (frame == nullptr) {
            break;
        }

        cv::Mat img = cv::Mat(frame->height, frame->width, CV_8U, frame->data[0]);
        cv::imshow("img", img);

        cv::waitKey(10);

        printf("%d, width=%d, height=%d, pts=%lld, pkt_dts=%lld\n", frame_count, frame->width, frame->height, frame->pts, frame->pkt_dts);
        xreader.dequeue_frame(&frame);

        //std::this_thread::sleep_for(std::chrono::milliseconds(25));
        frame_count++;
        if (frame_count >= 1024) {
            break;
        }
    }

    xreader.close_file();
    cv::destroyAllWindows();


    std::cout << "wait key..." << std::endl;
    std::getchar(); 

 
 

    std::cout << "bye!" << std::endl;

    return 0;
}

