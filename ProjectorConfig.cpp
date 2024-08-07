//
// Created by Simon Hetzer on 05.07.2024.
//

#include "ProjectorConfig.h"

VideoCapture ProjectorConfig::camera;
uint ProjectorConfig::CAMWIDTH, ProjectorConfig::CAMHEIGHT;
unsigned int ProjectorConfig::VAO;
unsigned int ProjectorConfig::EBO;
unsigned int ProjectorConfig::VBO;
unsigned int ProjectorConfig::shader;
GLuint ProjectorConfig::texture;

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
    auto testImg = getCameraImage();
    CAMHEIGHT = testImg.rows;
    CAMWIDTH = testImg.cols;
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
        waitKey(PATTERN_DELAY);

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
    white = captured.front();
}

Mat ProjectorConfig::decodeGraycode() {
    // Load the black and white captures from their predefined positions
    white = captured.front();
    captured.erase(captured.begin());
    Mat black = captured.back();
    captured.pop_back();

    Mat viz = Mat::zeros(CAMHEIGHT, CAMWIDTH, CV_8UC3);
    c2pList = std::vector<C2P>();

    // Decode each pixel
    uint pxlCount = 0, thresholdFailCount = 0, projPxlCount = 0, mappedPxlCount = 0;
    for (int y = 0; y < CAMHEIGHT; y++) {
        for (int x = 0; x < CAMWIDTH; x++) {
            cv::Point pixel;
            pxlCount++;
            bool thresholdPassed = white.at<cv::uint8_t>(y, x) - black.at<cv::uint8_t>(y, x) >
                                   BLACKTHRESHOLD;
            if (!thresholdPassed) thresholdFailCount++;
            try {
                bool projPixel = pattern->getProjPixel(captured, x, y, pixel);
                if (projPixel) projPxlCount++;
                if (thresholdPassed && projPixel)
                {
                    mappedPxlCount++;
                    viz.at<cv::Vec3b>(y,x)[0] = ((float) pixel.x / params.width) * 255;
                    viz.at<cv::Vec3b>(y,x)[1] = ((float) pixel.y / params.height) * 255;
                }
            }
            catch (Exception e) {
                std::cerr << "Tried decoding graycodes before pattern was initialized! Make sure to call generateGraycodes() before decodeGraycode()!" << std::endl;
                return viz; // empty
            }
        }
    }
    std::cout << "Threshold failed for " << thresholdFailCount << " of " << pxlCount <<
              " pixels (" << (float)thresholdFailCount / pxlCount * 100.0f << " %)." << std::endl;

    std::cout << "No mapping retrieved for " << pxlCount - projPxlCount << " of " << pxlCount <<
              " pixels (" << (float)(pxlCount - projPxlCount) / pxlCount * 100.0f << " %)." << std::endl;

    std::cout << mappedPxlCount << " of " << pxlCount <<
              " pixels (" << (float)(mappedPxlCount) / pxlCount * 100.0f << " %) were successfully mapped." << std::endl;

    // DENOISE THE IMAGE BEFORE CONVERTING IT TO C2P COORDINATES
    //imwrite("captured" + std::to_string(params.id) + "/noisy.png", viz);
    viz = reduceCalibrationNoise(viz);
    //imwrite("captured" + std::to_string(params.id) + "/denoised.png", viz);

    for (int y = 0; y < CAMHEIGHT; y++) {
        for (int x = 0; x < CAMWIDTH; x++) {
            int px = viz.at<Vec3b>(y,x)[0];
            int py = viz.at<Vec3b>(y,x)[1];
            px = ((float)px / 255) * params.width;
            py = ((float)py / 255) * params.height;
            c2pList.push_back(C2P(x, y, px, py));
        }
    }

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

void ProjectorConfig::loadC2Plist() {
    std::ifstream file("captured" + std::to_string(params.id) + "/c2p.csv");
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Could not open the c2p file!" << std::endl;
        return;
    }

    Mat viz = Mat::zeros(CAMHEIGHT, CAMWIDTH, CV_8UC3);
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string item;
        int data[4];
        int index = 0;

        while (std::getline(ss, item, ',')) {
            data[index++] = std::stoi(item);
        }

        if (index == 4) {
            c2pList.emplace_back(data[0], data[1], data[2], data[3]);
            viz.at<cv::Vec3b>(data[1], data[0])[0] = ((float)data[2] / params.width) * 255;
            viz.at<cv::Vec3b>(data[1], data[0])[1] = ((float)data[3] / params.height) * 255;
        } else {
            std::cerr << "Malformed line in CSV file: " << line << std::endl;
        }
    }

    imshow("Calibration", viz);
    waitKey(0);
    file.close();
}

Mat ProjectorConfig::reduceCalibrationNoise(const Mat& calib) {
    Mat eroded, dilated;
    int morphSize = 1;
    auto kernelSize1 = getStructuringElement(MORPH_RECT, Size(2 * morphSize + 1, 2 * morphSize + 1), Point(morphSize, morphSize));
    morphSize = 2;
    auto kernelSize2 = getStructuringElement(MORPH_RECT, Size(2 * morphSize + 1, 2 * morphSize + 1), Point(morphSize, morphSize));
    morphSize = 3;
    auto kernelSize3 = getStructuringElement(MORPH_RECT, Size(2 * morphSize + 1, 2 * morphSize + 1), Point(morphSize, morphSize));

    cv::dilate(calib, dilated, kernelSize3);
    cv::erode(dilated, eroded, kernelSize2);
    return eroded;
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

// ----------------------- CONSTRUCTORS --------------------------------------------
ProjectorConfig::ProjectorConfig() : params(ProjectorParams()), window(nullptr), shouldClose(false) {}

ProjectorConfig::ProjectorConfig(ProjectorParams p) : params(p), window(nullptr), shouldClose(false) {}

ProjectorConfig::ProjectorConfig(uint id, const ProjectorConfig* shared) : window(nullptr), shouldClose(false) {
    int count;
    auto monitors = glfwGetMonitors(&count);
    if (id >= count) {
        std::cerr << "Tried initializing projector with ID " << id << ", but that monitor does not exist!" << std::endl;
    }

    GLFWmonitor* monitor = monitors[id];
    const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
    int xPos, yPos;
    glfwGetMonitorPos(monitor, &xPos, &yPos);
    params = ProjectorParams(id, vidMode->width, vidMode->height, xPos, yPos);
    std::cout << "Creating full screen window on monitor #" << id << " (" << vidMode->width << " x " << vidMode->height << " px) at "
              << xPos << "/" << yPos << std::endl;

    initWindow((shared == nullptr) ? nullptr : shared->window);
}

// ---------------------- OPENGL FUNCTIONS ------------------------------
void ProjectorConfig::projectImage(const Mat &img, bool warp) {
    // Warp the image with the homography matrix
    Size resolution(params.width, params.height);
    Mat warpedImage = img;
    if (warp) {
        applyContributionMatrix(img, warpedImage);
        warpPerspective(warpedImage, warpedImage, homography, resolution);
    }

    // Create projector window
    glfwMakeContextCurrent(window);

    // Flip vertically
    //flip(warpedImage, warpedImage, 0);
    // Upload the image to the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, warpedImage.cols, warpedImage.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, warpedImage.ptr());
    // Render
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Swap front and back buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
    // Queried by managing application
    shouldClose = glfwWindowShouldClose(window);
}

void ProjectorConfig::projectImage(ProjectorConfig* projectors, uint count, const Mat& img) {
    bool shouldClose = false;
    while (!shouldClose) {
        for (int i = 0; i < count; i++) {
            if (projectors[i].shouldClose) shouldClose = true;
            projectors[i].projectImage(img, true);
        }
    }
}

bool ProjectorConfig::initWindow(GLFWwindow* shared) {
    // Get monitor
    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);

    // Create window
    window = glfwCreateWindow(params.width, params.height, "Projector", nullptr, shared);
    if (!window) {
        std::cerr << "Could not create GLFW window!" << std::endl;
        return false;
    }
    // Set window position
    glfwSetWindowPos(window,params.posX, params.posY);
    // Set event callbacks
    glfwSetKeyCallback(window, keyCallback);

    glfwMakeContextCurrent(window);

    // Either we have shared state or we need to create it
    if (shared == nullptr) {
        std::cout << "No shared OpenGL context, creating..." << std::endl;
        // glad: load all OpenGL function pointers
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return false;
        }

        // Prepare texture
        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        // Create buffer objects, array object and linked shader program
        VBO = createVertexBuffer();
        EBO = createElementBuffer();
        shader = createShaderProgram();
    }
    // Buffers objects only need to be created once and are shared, but context state needs to be set for each context
    VAO = createVertexArray(VBO, EBO);

    return true;
}

unsigned int ProjectorConfig::createVertexBuffer() {
    // Vertex data in 3D normalized device coordinates (-1,1)
    // Everything outside (-1,1) range AFTER vertex shader, will be clipped!
    float vertices[] = {
            // positions                      // texture coords
            1.0f,  1.0f, 0.0f,   1.0f, 1.0f,   // top right
            1.0f, -1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
            -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,   // bottom left
            -1.0f,  1.0f, 0.0f,   0.0f, 1.0f    // top left
    };

    // Create OpenGL buffer object and save its ID
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    // Bind to target of specific type "GL_ARRAY_BUFFER" for vertex buffer objects
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Because of binding calls to "GL_ARRAY_BUFFER" will now redirect to our bound "VBO" buffer, where we place the data
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                 GL_STATIC_DRAW); // STATIC_DRAW = data set once, used often
    // Unbind buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return VBO;
}

unsigned int ProjectorConfig::createElementBuffer() {
    // Index data pointing to the vertices in VBO
    unsigned int indices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
    };

    // Create OpenGL Element Buffer object and save its ID
    unsigned int EBO;
    glGenBuffers(1, &EBO);
    // Bind and set the buffer data to the indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    // Unbind
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return EBO;
}

unsigned int ProjectorConfig::createVertexArray(unsigned int vertexBuffer, unsigned int elementBuffer) {
    // Create OpenGL vertex array object and save its ID
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    // Bind the vertex array
    glBindVertexArray(VAO); {
        // Bind the buffers belonging to this vertex array
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);            // Contains the vertex data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);   // Contains the order in which to visit vertices
        // Set Vertex Attribute Pointers (tells the VAO how to use currently bound VBO data)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);// Set Vertex Attribute Pointers (tells the VAO how to use currently bound VBO data)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // Unbind the VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    // Unbind the VAO
    glBindVertexArray(0);
    // Unbind the EBO after(!) the VAO so the binding is saved
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    return VAO;
}

unsigned int ProjectorConfig::compileShader(const char* shaderSource, int shaderType) {
    unsigned int shader;
    shader = glCreateShader(shaderType);
    // Attach shader source code and compile
    glShaderSource(shader, 1, &shaderSource, nullptr);
    glCompileShader(shader);
    // Check if compilation was successful
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR! Shader compilation failed. Log:\n" << infoLog << std::endl;
    }
    return shader;
}

unsigned int ProjectorConfig::createShaderProgram() {
    unsigned int vertexShader = compileShader(VERTEXSHADERSOURCE, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(FRAGMENTSHADERSOURCE, GL_FRAGMENT_SHADER);

    // Create an OpenGL shader program object and save its ID
    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    // Attach shaders and link
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // Check for link errors
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR! Shader program compilation failed. Log:\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

bool ProjectorConfig::initGLFW() {
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For macOS compatibility

    return true;
}

void ProjectorConfig::errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

void ProjectorConfig::keyCallback(GLFWwindow *window, int key, int scandone, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void ProjectorConfig::computeContributions(ProjectorConfig *projectors, int count) {
    // Initialize 2 dimensional arrays for contribution matrix
    for (int i = 0; i < count; i++) {
        projectors[i].contributionMatrix = Mat_<float>(CAMHEIGHT, CAMWIDTH);
    }

    // CAREFUL! Looping over C2PList assumes they are all in the same order, CONFIRM THIS!!
    for (int pxl = 0; pxl < projectors[0].c2pList.size(); pxl++) {
        uint contributorsCount = 0;
        uint whiteAcc = 0.0f;
        Point pixel(projectors[0].c2pList[pxl].cx, projectors[0].c2pList[pxl].cy);
        // Loop over all projectors and check if they contribute to the pixel
        for (int i = 0; i < count; i++) {
            if (projectors[i].c2pList[pxl].px + projectors[i].c2pList[pxl].py != 0) {
                // This projector contributes to the camera pixel
                whiteAcc += projectors[i].white.at<cv::uint8_t>(pixel.y, pixel.x);
            }
        }

        // Loop over all projectors and set the pixel's contribution value
        for (int i = 0; i < count; i++) {
            float contribution = 0.0f;
            if ((whiteAcc > 0) && (projectors[i].c2pList[pxl].px + projectors[i].c2pList[pxl].py != 0)) {
                contribution = float(projectors[i].white.at<cv::uint8_t>(pixel.y, pixel.x)) / whiteAcc;
            }
            projectors[i].contributionMatrix.at<float>(pixel.y, pixel.x) = contribution;
        }
    }
}

void ProjectorConfig::visualizeContribution() {
    // For testing: visualize contribution
    Mat viz;
    contributionMatrix.convertTo(viz, CV_8UC1, 255.0);
    imshow("Contribution Projector" + std::to_string(params.id), viz);
    waitKey(0);
    imwrite("captured" + std::to_string(params.id) + "/contribution.png", viz);
}

void ProjectorConfig::applyContributionMatrix(const Mat& img, Mat& result) {
    Mat floatImage;
    img.convertTo(floatImage, CV_32FC3, 1.0f/255.0f);

    std::vector<Mat> channels(3);
    split(floatImage, channels);

    for (auto& channel : channels) {
        multiply(channel, contributionMatrix, channel);
    }

    merge(channels, result);
    result.convertTo(result, CV_8UC3, 255.0);
}

