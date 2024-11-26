#include "media/h264_encoder.h"


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

    H264Encoder xenc;

 
    std::cout << "wait key..." << std::endl;
    std::getchar(); 
 
 

    std::cout << "bye!" << std::endl;

    return 0;
}
































