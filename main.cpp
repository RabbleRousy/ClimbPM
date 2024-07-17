#include "ProjectorConfig.h"

int main()
{
    if (!ProjectorConfig::initGLFW()) return -1;

    auto calibrationResult = imread("captured1/result.png");
    Mat eroded, dilated;
    int morphSize = 1;
    auto kernelSize1 = getStructuringElement(MORPH_RECT, Size(2 * morphSize + 1, 2 * morphSize + 1), Point(morphSize, morphSize));
    morphSize = 2;
    auto kernelSize2 = getStructuringElement(MORPH_RECT, Size(2 * morphSize + 1, 2 * morphSize + 1), Point(morphSize, morphSize));
    morphSize = 3;
    auto kernelSize3 = getStructuringElement(MORPH_RECT, Size(2 * morphSize + 1, 2 * morphSize + 1), Point(morphSize, morphSize));

    cv::dilate(calibrationResult, dilated, kernelSize3);
    cv::erode(dilated, eroded, kernelSize2);

    imwrite("eroded.png", eroded);
    imwrite("dilated.png", dilated);
    return 0;


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


