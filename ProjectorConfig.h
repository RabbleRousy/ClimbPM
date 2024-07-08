//
// Created by Simon Hetzer on 05.07.2024.
//

#ifndef CLIMBPM_PROJECTORCONFIG_H
#define CLIMBPM_PROJECTORCONFIG_H

#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/structured_light.hpp>
#include <fstream>
#include <filesystem>
#ifdef __APPLE__
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::filesystem;
#endif

#define GRAYCODEWIDTHSTEP 50
#define GRAYCODEHEIGHTSTEP 50
#define WHITETHRESHOLD 5
#define BLACKTHRESHOLD 10

using namespace cv;

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

struct ProjectorParams {
    ushort id;
    uint width;
    uint height;
    uint posX;
    uint posY;
    ProjectorParams(ushort id, uint w, uint h, uint x, uint y)
    : id(id), width(w), height(h), posX(x), posY(y) { }
};

class ProjectorConfig {
public:
    // Generates the graycode pattern object and images to be projected
    void generateGraycodes();
    // Projects the graycode pattern and saves captured images
    void captureGraycodes();
    // Loads previously captured graycode projection images from files
    void loadGraycodes();
    // Decodes the captured images and generates c2pList, returns visualization
    Mat decodeGraycode();
    Mat getHomography();
    explicit ProjectorConfig(ProjectorParams p);

private:
    static VideoCapture camera;
    // Parameters of this projector
    ProjectorParams params;
    Ptr<structured_light::GrayCodePattern> pattern;
    // The graycode pattern images (not projected)
    std::vector<Mat> graycodes;
    // Captured images of projecting the graycodes from this projector
    std::vector<Mat> captured;
    // The camera-to-projector config of this projector
    std::vector<C2P> c2pList;
    // Homography matrix computed from c2p list
    Mat homography;
    void computeHomography();
    static void initCamera();
    static Mat getCameraImage();
};


#endif //CLIMBPM_PROJECTORCONFIG_H
