#include "ProjectorConfig.h"

int main()
{
    if (!ProjectorConfig::initGLFW()) return -1;

    ProjectorConfig::CAMHEIGHT = 1080;
    ProjectorConfig::CAMWIDTH = 1920;

    //ProjectorConfig::initCamera();

    const int PROJECTORCOUNT = 2;
    ProjectorConfig* projectors = new ProjectorConfig[PROJECTORCOUNT];

    // Define the projector configuration
    projectors[0] = ProjectorConfig(1);
    projectors[1] = ProjectorConfig(2, projectors);
    // Calibrate projectors
    for (int i = 0; i < PROJECTORCOUNT; i++) {
        std::cout << "Calibrating projector " << i + 1 << " ..." << std::endl;

        // Initialize
        projectors[i].generateGraycodes();

        // CAPTURE OR LOAD GRAYCOADES

        // All other projectors should show black
        /*
        Mat black = Mat::zeros(ProjectorConfig::CAMHEIGHT, ProjectorConfig::CAMWIDTH, CV_8UC3);
        for (int j = 0; j < PROJECTORCOUNT; j++) {
            if (j == i) continue;
            projectors[j].projectImage(black, false);
        }
        projectors[i].captureGraycodes();*/
        projectors[i].loadGraycodes();

        // DECODE OR LOAD DECODED CONFIG
        //projectors[i].decodeGraycode();
        // TODO: loadConfiguration() instead!
        projectors[i].loadC2Plist();

        std::cout << " Calibration finished." << std::endl;
    }

    // Calculate and visualize contribution of each pixel
    std::cout << "Computing individual contributions..." << std::endl;
    ProjectorConfig::computeContributions(projectors, PROJECTORCOUNT);

    auto testImg = imread("../Resources/test-image.jpg");
    ProjectorConfig::projectImage(projectors, PROJECTORCOUNT, testImg);

    delete [] projectors;

    glfwTerminate();
    return 0;
}


