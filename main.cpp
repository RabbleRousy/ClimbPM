#include <iostream>
#include <vector>
#include <sstream>

#include <opencv2/opencv.hpp>			// checked at opencv 3.4.1
#include <opencv2/structured_light.hpp>	// opencv contrib version
#include <fstream>
#include <filesystem>

#define PROJECTORWIDTH 1920
#define PROJECTORHEIGHT 1080
#define SCREENPOSX 3840
#define SCREENPOSY 0
#define GRAYCODEWIDTHSTEP 50
#define GRAYCODEHEIGHTSTEP 50
#define GRAYCODEWIDTH PROJECTORWIDTH / GRAYCODEWIDTHSTEP
#define GRAYCODEHEIGHT PROJECTORHEIGHT / GRAYCODEHEIGHTSTEP
#define WHITETHRESHOLD 5
#define BLACKTHRESHOLD 10

cv::VideoCapture camera;

// Initial camera (using OpenCV, this can modify to the different camera)
void initializeCamera()
{
    camera = cv::VideoCapture(0, cv::CAP_DSHOW);
    assert(camera.isOpened());
}
cv::Mat getCameraImage()
{
    // Capture a frame from camera
    cv::Mat image;
    camera.read(image);
    return image.clone();
}
void terminateCamera() {}

// Camera to Projector
struct C2P {
    int cx;
    int cy;
    int px;
    int py;
    C2P(int camera_x, int camera_y, int proj_x, int proj_y)
    {
        cx = camera_x;
        cy = camera_y;
        px = proj_x;
        py = proj_y;
    }
};

int main()
{
    // -----------------------------------
    // ----- Prepare graycode images -----
    // -----------------------------------
    cv::structured_light::GrayCodePattern::Params params;
    params.width = PROJECTORWIDTH;
    params.height = PROJECTORHEIGHT;
    auto pattern = cv::structured_light::GrayCodePattern::create(params);

    pattern->setWhiteThreshold(WHITETHRESHOLD);
    pattern->setBlackThreshold(BLACKTHRESHOLD);

    std::vector<cv::Mat> graycodes;
    pattern->generate(graycodes);
    std::cout << "Generated the " << graycodes.size() << " graycode patterns!" << std::endl;

    cv::Mat blackCode, whiteCode;
    pattern->getImagesForShadowMasks(blackCode, whiteCode);
    graycodes.push_back(blackCode);
    graycodes.push_back(whiteCode);
    std::cout << "Generated 2 more (fully black and white) patterns!" << std::endl;

    // -----------------------------
    // ----- Prepare cv window -----
    // -----------------------------
    cv::namedWindow("Pattern", cv::WINDOW_NORMAL);
    cv::resizeWindow("Pattern", PROJECTORWIDTH, PROJECTORHEIGHT);
    // This is the setting to display on the second display,
    // please adjust SCREENPOSX, SCREENPOSY
    cv::moveWindow("Pattern", SCREENPOSX, SCREENPOSY);
    cv::setWindowProperty("Pattern", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);

    // ----------------------------------
    // ----- Wait camera adjustment -----
    // ----------------------------------
    initializeCamera();

    // Show white image first
    cv::imshow("Pattern", graycodes[graycodes.size() - 1]);
    while (true) {
        cv::Mat img = getCameraImage();
        cv::imshow("camera", img);
        if (cv::waitKey(1) != -1) break;
    }

    // --------------------------------
    // ----- Capture the graycode -----
    // --------------------------------
    std::filesystem::create_directory("captured");
    std::vector<cv::Mat> captured;
    for (int i = 0; i < graycodes.size(); i++) {
        auto gimg = graycodes[i];
        cv::imshow("Pattern", gimg);
        // Display an image (graycode) on the display > wait until reflected in the camera buffer
        // This requires waiting time depends on the camera
        cv::waitKey(5000);

        cv::Mat img = getCameraImage();
        // Convert to grayscale
        cv::Mat grayImg;
        cv::cvtColor(img, grayImg, cv::COLOR_BGR2GRAY);

        /* SAVING IMAGES TO DISK
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << i;
        if (!cv::imwrite("captured/cam_" + oss.str() + ".png", grayImg))
            std::cerr << "Error saving image!" << std::endl;*/

        captured.push_back(grayImg);
    }

    terminateCamera();

    // -------------------------------
    // ----- Decode the graycode -----
    // -------------------------------
    cv::Mat white = captured.front();
    captured.erase(captured.begin());
    cv::Mat black = captured.back();
    captured.pop_back();

    int camHeight = captured[0].rows;
    int camWidth = captured[0].cols;

    std::cout << "Camera resolution is " << camWidth << " x " << camHeight << " px." << std::endl;
    std::cout << "Camera image channels: " << captured[0].channels() << std::endl;

    cv::Mat viz = cv::Mat::zeros(camHeight, camWidth, CV_8UC3);
    std::vector<C2P> c2pList;

    uint pxlCount, thresholdFailCount, projPxlCount, mappedPxlCount = 0;
    for (int y = 0; y < camHeight; y++) {
        for (int x = 0; x < camWidth; x++) {
            cv::Point pixel;
            pxlCount++;
            bool thresholdPassed = white.at<cv::uint8_t>(y, x) - black.at<cv::uint8_t>(y, x) >
                                   BLACKTHRESHOLD;
            if (!thresholdPassed) thresholdFailCount++;
            bool projPixel = pattern->getProjPixel(captured, x, y, pixel);
            if (projPixel) projPxlCount++;
            if (thresholdPassed && projPixel)
            {
                mappedPxlCount++;
                viz.at<cv::Vec3b>(y,x)[0] = ((float) pixel.x / PROJECTORWIDTH) * 255;
                viz.at<cv::Vec3b>(y,x)[1] = ((float) pixel.y / PROJECTORHEIGHT) * 255;
                c2pList.push_back(C2P(x, y, pixel.x,
                                      pixel.y));
            }
        }
    }
    std::cout << "Threshold failed for " << thresholdFailCount << " of " << pxlCount <<
              " pixels (" << (float)thresholdFailCount / pxlCount * 100.0f << " %)." << std::endl;

    std::cout << "No mapping retrieved for " << pxlCount - projPxlCount << " of " << pxlCount <<
              " pixels (" << (float)(pxlCount - projPxlCount) / pxlCount * 100.0f << " %)." << std::endl;

    std::cout << mappedPxlCount << " of " << pxlCount <<
              " pixels (" << (float)(mappedPxlCount) / pxlCount * 100.0f << " %) were successfully mapped." << std::endl;

    // ---------------------------
    // ----- Save C2P as csv -----
    // ---------------------------
    std::ofstream os("c2p.csv");
    for (auto elem : c2pList) {
        os << elem.cx << ", " << elem.cy << ", " << elem.px << ", " << elem.py
           << std::endl;
    }
    os.close();

    // ----------------------------
    // ----- Visualize result -----
    // ----------------------------
    cv::imshow("result", viz);
    cv::waitKey(0);

    // Save result
    if (!cv::imwrite("captured/result.png", viz))
        std::cerr << "Error saving result image!" << std::endl;

    return 0;
}