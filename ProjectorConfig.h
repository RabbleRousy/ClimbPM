//
// Created by Simon Hetzer on 05.07.2024.
//

#ifndef CLIMBPM_PROJECTORCONFIG_H
#define CLIMBPM_PROJECTORCONFIG_H

#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/structured_light.hpp>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <fstream>
#include <filesystem>
#ifdef __APPLE__
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::filesystem;
#endif

#define WHITETHRESHOLD 80
#define BLACKTHRESHOLD 20
#define PATTERN_DELAY 5000

// Shader code (nothing fancy, basic texturing)
#define VERTEXSHADERSOURCE "#version 330 core\nlayout (location = 0) in vec3 aPos;\nlayout (location = 1) in vec2 aTexCoord;\nout vec2 texCoord;\nvoid main()\n{\ngl_Position = vec4(aPos, 1.0);\ntexCoord = aTexCoord;\n}\0"
#define FRAGMENTSHADERSOURCE "#version 330 core\nout vec4 FragColor;\nin vec2 texCoord;\nuniform sampler2D ourTexture;\nvoid main()\n{\nFragColor = texture(ourTexture, texCoord);\n}\n"

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
    ProjectorParams() : id(0), width(0), height(0), posX(0), posY(0) {}
};

class ProjectorConfig {
public:
    // -------- STATIC VARIABLES ---------
    static uint CAMHEIGHT, CAMWIDTH;

    // -------- STATIC FUNCTIONS ---------
    static bool initGLFW();
    static void initCamera();
    static void computeContributions(ProjectorConfig* projectors, int count);
    static void projectImage(ProjectorConfig* projectors, uint count, const Mat& img);
    static void computeBrightnessMap(ProjectorConfig* projectors, int count);

    // -------- MEMBER FUNCTIONS ------
    bool wantsToClose() { return shouldClose; }
    // Generates the graycode pattern object and images to be projected
    void generateGraycodes();
    // Projects the graycode pattern and saves captured images
    void captureGraycodes();
    // Loads previously captured graycode projection images from files
    void loadGraycodes();
    // Decodes the captured images and generates c2pList, returns visualization
    Mat decodeGraycode();
    Mat getHomography();
    Mat warpImage(Mat img, bool save = false);
    void projectImage(Mat img, bool warp);
    void visualizeContribution();
    // Initializes the configuration from existing files
    void loadConfiguration();
    void applyAreaMask();

    // ----- CONSTRUCTORS ----------
    ProjectorConfig();
    explicit ProjectorConfig(ProjectorParams p);
    explicit ProjectorConfig(uint id, const ProjectorConfig* shared = nullptr);

private:
    // ----- STATIC VARIABLES ------
    static VideoCapture camera;
    static Mat brightnessMap; // unused
    // Shared OpenGL resources
    static unsigned int EBO, VBO, VAO;
    static unsigned int shader;
    static GLuint texture;

    // ------ STATIC FUNCTIONS ---------------------------
    static void getProjectionBoundaries(int count, int& minX, int& minY, int& maxX, int& maxY); // unused
    static void keyCallback(GLFWwindow* window, int key, int scandone, int action, int mods);
    static void errorCallback(int error, const char* description);
    static Mat getCameraImage();

    // ----- MEMBER VARIABLES -------
    // Whether our window wants to be close
    bool shouldClose;
    // GLFW Window
    GLFWwindow* window;
    // Parameters of this projector
    ProjectorParams params;
    Ptr<structured_light::GrayCodePattern> pattern;
    // The graycode pattern images (not projected)
    std::vector<Mat> graycodes;
    // Captured images of projecting the graycodes from this projector
    std::vector<Mat> captured;
    Mat white;
    // The camera-to-projector config of this projector
    std::vector<C2P> c2pList;
    // Homography matrix computed from c2p list
    Mat homography;
    // Matrix containing the shared contribution to each pixel in camera space
    Mat contributionMatrix; // unused

    // ------------ MEMBER FUNCTIONS -----------------------
    void applyContributionMatrix(const Mat& img, Mat& result); // unused
    Mat reduceCalibrationNoise(const Mat& calib);
    void computeHomography();
    Mat computeProjectorAreaMask(const Mat& whiteImg);
    // Loads the C2P list from .csv file
    Mat loadC2Plist();
    // Loads contribution matrix from file
    void loadContribution();
    bool initWindow(GLFWwindow* shared = nullptr);

    // ------- OPENGL HELPER FUNCTIONS ----------
    unsigned int createVertexBuffer();
    unsigned int createElementBuffer();
    unsigned int createVertexArray(unsigned int vertexBuffer, unsigned int elementBuffer);
    unsigned int compileShader(const char* shaderSource, int shaderType);
    unsigned int createShaderProgram();
};


#endif //CLIMBPM_PROJECTORCONFIG_H
