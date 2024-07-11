#include "ProjectorConfig.h"

void errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

int main()
{
    glfwInit();
    glfwSetErrorCallback(errorCallback);
    // Define the projector configuration
    ProjectorConfig projector1 = ProjectorConfig(2);

    /*
    // Graycode patterns NEEDS to be generated, even when loading from disk!!
    projector1.generateGraycodes();
    // Either capture or load graycode images from this projector
    //projector1.captureGraycodes();
    projector1.loadGraycodes();
    // Decode (optionally retrieve visualization matrix
    projector1.decodeGraycode();

    std::cout << "Homography Matrix:\n" << projector1.getHomography() << std::endl;
    */
    auto testImg = imread("../Resources/test-image.jpg");
    projector1.projectImage(testImg);

    return 0;
}


