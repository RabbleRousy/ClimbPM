#include "ProjectorConfig.h"

int main()
{
    if (!ProjectorConfig::initGLFW()) return -1;

    const int PROJECTORCOUNT = 2;
    ProjectorConfig* projectors = new ProjectorConfig[PROJECTORCOUNT];

    // Define the projector configuration
    projectors[0] = ProjectorConfig(1);
    projectors[1] = ProjectorConfig(2, projectors+1);
    // Calibrate projectors
    for (int i = 0; i < PROJECTORCOUNT; i++) {
        projectors[i].generateGraycodes();
        projectors[i].captureGraycodes();
        projectors[i].decodeGraycode();
    }
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


