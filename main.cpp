#include "ProjectorConfig.h"

int main()
{
    // Define the projector configuration
    ProjectorConfig projector1 = ProjectorConfig(ProjectorParams(1, 1920, 1080, 3840, 0));
    // Graycode patterns NEEDS to be generated, even when loading from disk!!
    projector1.generateGraycodes();
    // Either capture or load graycode images from this projector
    projector1.captureGraycodes();
    //projector1.loadGraycodes();
    // Decode and retrieve the visualization matrix
    auto viz = projector1.decodeGraycode();

    // ----------------------------
    // ----- Visualize result -----
    // ----------------------------
    //cv::imshow("result", viz);
    //cv::waitKey(0);

    std::cout << "Homography Matrix:\n" << projector1.getHomography() << std::endl;

    auto testImg = imread("../Resources/test-image.jpg");
    projector1.projectImage(testImg);

    return 0;
}


