#include "media/mp4_decoder.h"


using namespace duck::media;

void test_yuv420p(cv::Mat img); 

int main(int argc, char* argv[])
{
    google::InstallFailureSignalHandler();
    google::InitGoogleLogging(argv[0]);

    if (argc != 3) {
        printf("usage: %s *.png (0:INFO, 1:WARNING, 2:ERROR, 3:FATAL)\n", argv[0]);
        return -1;
    }

    std::string img_name = argv[1];
    FLAGS_stderrthreshold = atoi(argv[2]);
    FLAGS_minloglevel = 0;

    cv::Mat img = cv::imread(img_name);

    test_yuv420p(img);

    // std::cout << "wait key..." << std::endl;
    // std::getchar(); 
 
    // std::cout << "bye!" << std::endl;

    return 0;
}

void test_yuv420p(cv::Mat img) 
{
    cv::Mat yuv420p_img;
    cv::cvtColor(img, yuv420p_img, cv::COLOR_BGR2YUV_I420);

    printf("width=%d, height=%d, channel=%d\n", yuv420p_img.cols, yuv420p_img.rows, yuv420p_img.channels());

    cv::Mat yuv420p_y_img = yuv420p_img(cv::Rect(0, 0, img.cols, img.rows)).clone();
    uchar* yuv420p_u_data = (uchar*)yuv420p_img.data + img.cols * img.rows;
    uchar* yuv420p_v_data = (uchar*)yuv420p_img.data + img.cols * img.rows * 5 / 4;
    cv::Mat yuv420p_u_img = cv::Mat(img.rows / 2, img.cols / 2, CV_8U, yuv420p_u_data);
    cv::Mat yuv420p_v_img = cv::Mat(img.rows / 2, img.cols / 2, CV_8U, yuv420p_v_data);



    cv::imwrite("yuv420p_img.png", yuv420p_img);
    cv::imwrite("yuv420p_y_img.png", yuv420p_y_img); 
    cv::imwrite("yuv420p_u_img.png", yuv420p_u_img);
    cv::imwrite("yuv420p_v_img.png", yuv420p_v_img);

    cv::Mat bgr_img;
    cv::cvtColor(yuv420p_img, bgr_img, cv::COLOR_YUV2BGR_I420);
    cv::imwrite("bgr_img.png", bgr_img);
}

