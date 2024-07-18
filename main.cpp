#include "ProjectorConfig.h"

int main()
{
    if (!ProjectorConfig::initGLFW()) return -1;

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
            projectors[j].clear();
        }
        projectors[i].generateGraycodes();
        projectors[i].captureGraycodes();
        //projectors[i].decodeGraycode();
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


