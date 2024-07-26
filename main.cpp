#include "ProjectorConfig.h"

int main()
{
    if (!ProjectorConfig::initGLFW()) return -1;

    ProjectorConfig dummyProjector = ProjectorConfig(ProjectorParams(1, 1920, 1080, 0, 0));
    ProjectorConfig::CAMHEIGHT = 1080;
    ProjectorConfig::CAMWIDTH = 1920;

    dummyProjector.generateGraycodes();
    dummyProjector.loadC2Plist();

    return 0;

    ProjectorConfig::initCamera();

    const int PROJECTORCOUNT = 2;
    ProjectorConfig* projectors = new ProjectorConfig[PROJECTORCOUNT];

    // Define the projector configuration
    projectors[0] = ProjectorConfig(1);
    projectors[1] = ProjectorConfig(2, projectors);
    // Calibrate projectors
    for (int i = 0; i < PROJECTORCOUNT; i++) {
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

    delete [] projectors;
    return 0;

    // Calculate and visualize contribution of each pixel
    ProjectorConfig::computeContributions(projectors, PROJECTORCOUNT);
    for (int i = 0; i < PROJECTORCOUNT; i++) {
        projectors[i].visualizeContribution();
    }
    delete [] projectors;

    /*
    auto testImg = imread("../Resources/test-image.jpg");
    while (!projector1.shouldClose && !projector2.shouldClose) {
        projector1.projectImage(testImg);
        projector2.projectImage(testImg);
    }
     */
    glfwTerminate();
    return 0;
}


