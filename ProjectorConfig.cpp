//
// Created by Simon Hetzer on 05.07.2024.
//

#include "ProjectorConfig.h"

VideoCapture ProjectorConfig::camera;

void ProjectorConfig::generateGraycodes() {
    // Create pattern object
    structured_light::GrayCodePattern::Params grayCodeParams;
    grayCodeParams.width = params.width;
    grayCodeParams.height = params.height;
    pattern = structured_light::GrayCodePattern::create(grayCodeParams);
    pattern->setWhiteThreshold(WHITETHRESHOLD);
    pattern->setBlackThreshold(BLACKTHRESHOLD);

    // Populate graycode array
    graycodes = std::vector<Mat>();
    pattern->generate(graycodes);
    std::cout << "Generated the " << graycodes.size() << " graycode patterns!" << std::endl;
    cv::Mat blackCode, whiteCode;
    pattern->getImagesForShadowMasks(blackCode, whiteCode);
    graycodes.push_back(blackCode);
    graycodes.push_back(whiteCode);
    std::cout << "Generated 2 more (fully black and white) patterns!" << std::endl;
}

void ProjectorConfig::initCamera() {
    camera = VideoCapture(0, CAP_DSHOW);
    assert(camera.isOpened());
}

Mat ProjectorConfig::getCameraImage() {
    Mat image;
    camera.read(image);
    return image.clone();
}

void ProjectorConfig::captureGraycodes() {
    namedWindow("Pattern", WINDOW_NORMAL);
    resizeWindow("Pattern", params.width, params.height);
    moveWindow("Pattern", params.posX, params.posY);
    setWindowProperty("Pattern", WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);

    if (!camera.isOpened())
        initCamera();

    // Show white image first
    imshow("Pattern", graycodes[graycodes.size() - 1]);
    while (true) {
        Mat img = getCameraImage();
        imshow("camera", img);
        if (waitKey(1) != -1) break;
    }

    fs::create_directory("captured" + std::to_string(params.id));
    for (int i = 0; i < graycodes.size(); i++) {
        // Display the graycode
        Mat& gimg = graycodes[i];
        imshow("Pattern", gimg);
        waitKey(5000);

        Mat img = getCameraImage();
        // Convert to grayscale
        Mat grayImg;
        cvtColor(img, grayImg, COLOR_BGR2GRAY);

        // Save to disk
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << i;
        if (!imwrite("captured" + std::to_string(params.id) + "/cam_" + oss.str() + ".png", grayImg))
            std::cerr << "Error saving image!" << std::endl;

        // Save to img array
        captured.push_back(grayImg);
    }
}

void ProjectorConfig::loadGraycodes() {
    captured = std::vector<Mat>();
    std::string path = "captured" + std::to_string(params.id);
    int i = 0;
    while (true) {
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << i;
        std::string imgPath = path + "/cam_" + oss.str() + ".png";
        std::cout << "Loading image \"" << imgPath << "\" ..." << std::endl;
        if (!fs::exists(imgPath)) {
            std::cout << "Loaded " << i << " captured images from disk." << std::endl;
            break;
        }
        Mat img = imread(imgPath, IMREAD_GRAYSCALE);
        if (img.empty()) {
            std::cerr << "Could not open or find the image \"" << imgPath << "\"!" << std::endl;
        }
        captured.push_back(img);
        i = i + 1;
    }
}

Mat ProjectorConfig::decodeGraycode() {
    // Load the black and white captures from their predefined positions
    Mat white = captured.front();
    captured.erase(captured.begin());
    Mat black = captured.back();
    captured.pop_back();

    int camHeight = captured[0].rows;
    int camWidth = captured[0].cols;

    Mat viz = Mat::zeros(camHeight, camWidth, CV_8UC3);
    c2pList = std::vector<C2P>();

    // Decode each pixel
    uint pxlCount = 0, thresholdFailCount = 0, projPxlCount = 0, mappedPxlCount = 0;
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
                viz.at<cv::Vec3b>(y,x)[0] = ((float) pixel.x / params.width) * 255;
                viz.at<cv::Vec3b>(y,x)[1] = ((float) pixel.y / params.height) * 255;
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

    // Save C2P as file
    std::ofstream os("captured" + std::to_string(params.id) + "/c2p.csv");
    for (C2P elem : c2pList) {
        os << elem.cx << ", " << elem.cy << ", " << elem.px << ", " << elem.py << std::endl;
    }
    os.close();

    // Save result image
    if (!imwrite("captured" + std::to_string(params.id) + "/result.png", viz))
        std::cerr << "Error saving result image!" << std::endl;

    return viz;
}

Mat ProjectorConfig::getHomography() {
    if (homography.empty())
        computeHomography();
    return homography;
}

void ProjectorConfig::computeHomography() {
    if (c2pList.empty()) {
        std::cerr
                << "Tried computing homography matrix but C2P points have not been calculated! Try calling decodeGraycode() first!"
                << std::endl;
        return;
    }

    std::vector<Point2f> cameraPoints;
    std::vector<Point2f> projectorPoints;
    for (C2P point : c2pList) {
        cameraPoints.emplace_back(point.cx, point.cy);
        projectorPoints.emplace_back(point.px, point.py);
    }
    // Here is where the magic happens
    homography = findHomography(cameraPoints, projectorPoints, RANSAC);
}

ProjectorConfig::ProjectorConfig(ProjectorParams p) : params(p) {}

void ProjectorConfig::projectImage(const Mat &img) {
    // Warp the image with the homography matrix
    Size resolution(params.width, params.height);
    Mat warpedImage;
    warpPerspective(img, warpedImage, homography, resolution);

    // Create projector window
    std::string windowName = "Projector" + std::to_string(params.id);
    namedWindow(windowName, WINDOW_NORMAL);
    resizeWindow(windowName, params.width, params.height);
    moveWindow(windowName, params.posX, params.posY);
    setWindowProperty(windowName, WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);

    // Display warped image on the projector
    imshow(windowName, warpedImage);
    waitKey(100);
    // Display what the camera sees on the default monitor
    initCamera();
    waitKey(100);
    auto camImg = getCameraImage();
    imshow("camera", camImg);
    waitKey(0);
}

