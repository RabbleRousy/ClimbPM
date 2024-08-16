#include "ProjectorConfig.h"

int main()
{
    if (!ProjectorConfig::initGLFW()) return -1;

    ProjectorConfig::CAMHEIGHT = 1080;
    ProjectorConfig::CAMWIDTH = 1920;

    ProjectorConfig::initCamera();

    const int PROJECTORCOUNT = 3;
    ProjectorConfig* projectors = new ProjectorConfig[PROJECTORCOUNT];

    // Define the projector configuration
    projectors[0] = ProjectorConfig(1);
    projectors[1] = ProjectorConfig(2, projectors);
    projectors[2] = ProjectorConfig(3, projectors);

    ProjectorConfig::computeBrightnessMap(projectors, PROJECTORCOUNT);

    delete[] projectors;
    glfwTerminate();
    return 0;

    // Calibrate/load projector configurations
    for (int i = 0; i < PROJECTORCOUNT; i++) {

        // Load an existing configuration from folder "captured<i>"
        projectors[i].loadConfiguration();
        continue;

        // -------------- CALIBRATE NEW CONFIGURATION ----------------

        std::cout << "Calibrating projector " << i + 1 << " ..." << std::endl;
        // Initialize
        projectors[i].generateGraycodes();

        // All other projectors should show black
        Mat black = Mat::zeros(ProjectorConfig::CAMHEIGHT, ProjectorConfig::CAMWIDTH, CV_8UC3);
        for (int j = 0; j < PROJECTORCOUNT; j++) {
            if (j == i) continue;
            projectors[j].projectImage(black, false);
        }
        // Capture Graycodes
        projectors[i].captureGraycodes();

        // Decode captured Graycodes
        projectors[i].decodeGraycode();

        std::cout << " Calibration finished." << std::endl;

        // After last configuration, compute contributions
        if (i == PROJECTORCOUNT-1) {
            std::cout << "Computing individual contributions..." << std::endl;
            ProjectorConfig::computeContributions(projectors, PROJECTORCOUNT);
        }
    }

    // Close windows opened while calibrating
    destroyAllWindows();

    auto testImg = imread("../Resources/test-image.jpg");
    ProjectorConfig::projectImage(projectors, PROJECTORCOUNT, testImg);

    delete [] projectors;

    glfwTerminate();
    return 0;
}


