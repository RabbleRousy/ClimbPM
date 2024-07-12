#include "ProjectorConfig.h"

int main()
{
    if (!ProjectorConfig::initGLFW()) return -1;

    // Define the projector configuration
    ProjectorConfig projector1 = ProjectorConfig(1);
    ProjectorConfig projector2 = ProjectorConfig(2, &projector1);

    auto testImg = imread("../Resources/test-image.jpg");
    while (!projector1.shouldClose && !projector2.shouldClose) {
        projector1.projectImage(testImg);
        projector2.projectImage(testImg);
    }
    glfwTerminate();
    return 0;
}


