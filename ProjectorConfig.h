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

#define GRAYCODEWIDTHSTEP 50
#define GRAYCODEHEIGHTSTEP 50
#define WHITETHRESHOLD 5
#define BLACKTHRESHOLD 10
#define PATTERN_DELAY 500

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
    static uint CAMHEIGHT, CAMWIDTH;
    static bool initGLFW();
    static void computeContributions(ProjectorConfig* projectors, int count);
    bool shouldClose;
    // Generates the graycode pattern object and images to be projected
    void generateGraycodes();
    // Projects the graycode pattern and saves captured images
    void captureGraycodes();
    // Loads previously captured graycode projection images from files
    void loadGraycodes();
    // Decodes the captured images and generates c2pList, returns visualization
    Mat decodeGraycode();
    Mat getHomography();
    void projectImage(const Mat& img, bool warp);
    void visualizeContribution();
    // CONSTRUCTORS
    ProjectorConfig();
    explicit ProjectorConfig(ProjectorParams p);
    explicit ProjectorConfig(uint id, const ProjectorConfig* shared = nullptr);
    ~ProjectorConfig();

private:
    static VideoCapture camera;
    // GLFW Window
    GLFWwindow* window;
    // Shared OpenGL resources
    static unsigned int EBO, VBO, VAO;
    static unsigned int shader;
    static GLuint texture;
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
    // Matrix containing the shared contribution to each pixel
    float** contributionMatrix = nullptr;
    Mat reduceCalibrationNoise(const Mat& calib);
    void computeHomography();
    bool initWindow(GLFWwindow* shared = nullptr);
    static void errorCallback(int error, const char* description);
    static void keyCallback(GLFWwindow* window, int key, int scandone, int action, int mods);
    static void initCamera();
    static Mat getCameraImage();
    // ------- OpenGL Functions ----------
    unsigned int createVertexBuffer();
    unsigned int createElementBuffer();
    unsigned int createVertexArray(unsigned int vertexBuffer, unsigned int elementBuffer);
    unsigned int compileShader(const char* shaderSource, int shaderType);
    unsigned int createShaderProgram();
};


#endif //CLIMBPM_PROJECTORCONFIG_H
