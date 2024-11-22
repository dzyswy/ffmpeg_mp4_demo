#include "media/mp4_decoder.h"


using namespace duck::media;


int main(int argc, char* argv[])
{
    google::InstallFailureSignalHandler();
    google::InitGoogleLogging(argv[0]);

    if (argc != 3) {
        printf("usage: *.mp4 %s (0:INFO, 1:WARNING, 2:ERROR, 3:FATAL)\n", argv[0]);
        return -1;
    }

    std::string mp4_name = argv[1];
    FLAGS_stderrthreshold = atoi(argv[2]);
    FLAGS_minloglevel = 0;

    Mp4Decoder mp4x;
    mp4x.open_file(mp4_name);

    std::cout << "wait key..." << std::endl;
    std::getchar(); 
 

    std::cout << "bye!" << std::endl;

    return 0;
}
































