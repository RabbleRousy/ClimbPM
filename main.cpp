#include "ProjectorConfig.h"

void projectImage(ProjectorConfig* projectors, uint count, Mat img);

int main()
{
    if (!ProjectorConfig::initGLFW()) return -1;


    ProjectorConfig dummyProjector = ProjectorConfig(ProjectorParams(1, 1920, 1080, 0, 0));
    ProjectorConfig::CAMHEIGHT = 1080;
    ProjectorConfig::CAMWIDTH = 1920;

    //ProjectorConfig::initCamera();

    const int PROJECTORCOUNT = 2;
    ProjectorConfig* projectors = new ProjectorConfig[PROJECTORCOUNT];

    // Define the projector configuration
    projectors[0] = ProjectorConfig(ProjectorParams(1, 1920, 1080, 0, 0));
    projectors[1] = ProjectorConfig(ProjectorParams(2, 1920, 1080, 0, 0));
    // Calibrate projectors
    for (int i = 0; i < PROJECTORCOUNT; i++) {

        projectors[i].generateGraycodes();
        projectors[i].loadGraycodes(); // needed for the "white" image
        //projectors[i].decodeGraycode();
        projectors[i].loadC2Plist();

        std::cout << "Projector " << i+1 << " initialized." << std::endl;

        continue;

        // All other projectors should show black
        Mat black = Mat::zeros(ProjectorConfig::CAMHEIGHT, ProjectorConfig::CAMWIDTH, CV_8UC3);
        for (int j = 0; j < PROJECTORCOUNT; j++) {
            if (j == i) continue;
            projectors[j].projectImage(black, false);
        }
        projectors[i].generateGraycodes();

        // CAPTURE OR LOAD GRAYCOADES
        projectors[i].captureGraycodes();
        //projectors[i].loadGraycodes();

        // DECODE OR LOAD DECODED CONFIG
        projectors[i].decodeGraycode();
        //projectors[i].loadC2Plist();
    }

    // Calculate and visualize contribution of each pixel
    std::cout << "Computing individual contributions..." << std::endl;
    ProjectorConfig::computeContributions(projectors, PROJECTORCOUNT);

    for (int i = 0; i < PROJECTORCOUNT; i++) {
        projectors[i].visualizeContribution();
    }

    delete [] projectors;

    glfwTerminate();
    return 0;
}

void projectImage(ProjectorConfig* projectors, uint count, Mat img) {
    bool shouldClose = false;
    while (!shouldClose) {
        for (int i = 0; i < count; i++) {
            if (projectors[i].shouldClose) shouldClose = true;
            projectors[i].projectImage(img, true);
        }
    }
}


