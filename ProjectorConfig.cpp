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
    // Make sure we have a window for the projector
    if (!window) {
        if (!initWindow()) {
            std::cerr << "Failed to create a window to project the image to!" << std::endl;
            return;
        }
    }

    // Warp the image with the homography matrix
    Size resolution(params.width, params.height);
    Mat warpedImage;
    warpPerspective(img, warpedImage, homography, resolution);

    // Create projector window
    glfwMakeContextCurrent(window);
    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return;
    }
    // Create buffer objects, array object and linked shader program
    unsigned int VBO = createVertexBuffer();
    unsigned int EBO = createElementBuffer();
    unsigned int VAO = createVertexArray(VBO, EBO);
    unsigned int shader = createShaderProgram();
    // Prepare texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Display warped image on the projector
    while (!glfwWindowShouldClose(window)) {
        // Flip vertically
        flip(warpedImage, warpedImage, 0);
        // Upload the image to the texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, warpedImage.cols, warpedImage.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, warpedImage.ptr());
        // Render
        glClear(GL_COLOR_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // Swap front and back buffers
        glfwSwapBuffers(window);
        // Poll for and process events
        glfwPollEvents();
        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool ProjectorConfig::initWindow() {
    // Init GLFW
    if (!glfwInit()) {
        std::cerr << "GLFW could not be initialized!" << std::endl;
        return false;
    }
    // Get monitor
    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    if (params.id >= count) {
        std::cerr << "No monitor with id " << params.id << " found!" << std::endl;
        return false;
    }
    // Create window
    window = glfwCreateWindow(params.width, params.height, "Projector", monitors[params.id], NULL);
    if (!window) {
        std::cerr << "Could not create GLFW window!" << std::endl;
        return false;
    }
    return true;
}

unsigned int ProjectorConfig::createVertexBuffer() {
    // Vertex data in 3D normalized device coordinates (-1,1)
    // Everything outside (-1,1) range AFTER vertex shader, will be clipped!
    float vertices[] = {
            // positions                      // texture coords
            0.5f,  0.5f, 0.0f,   1.0f, 1.0f,   // top right
            0.5f, -0.5f, 0.0f,   1.0f, 0.0f,   // bottom right
            -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,   // bottom left
            -0.5f,  0.5f, 0.0f,   0.0f, 1.0f    // top left
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

unsigned int ProjectorConfig::createVertexShader() {
    const char* vertexShaderSource = "#version 330 core\n"
                                     "layout (location = 0) in vec3 aPos;\n"
                                     "layout (location = 1) in vec2 aTexCoord;\n"
                                     "out vec2 texCoord;\n"
                                     "void main()\n"
                                     "{\n"
                                     "  gl_Position = vec4(aPos, 1.0);\n"
                                     "  texCoord = aTexCoord;\n"
                                     "}\0";
    // Create OpenGL shader object and save its ID
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    // Attach shader source code and compile
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // Check if compilation was successful
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR! Vertex shader compilation failed. Log:\n" << infoLog << std::endl;
    }
    return vertexShader;
}

unsigned int ProjectorConfig::createFragmentShader() {
    const char* fragmentShaderSource = "#version 330 core\n"
                                       "out vec4 FragColor;\n"
                                       "in vec2 texCoord;\n"
                                       "uniform sampler2D ourTexture;\n"
                                       "void main()\n"
                                       "{\n"
                                       "    FragColor = texture(ourTexture, texCoord);\n"
                                       "}\n";
    // Create Fragment Shader Object and save its ID
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    // Attach shader source and compile
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // Check for compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR! Fragment shader compilation failed. Log:\n" << infoLog << std::endl;
    }
    return fragmentShader;
}

unsigned int ProjectorConfig::createShaderProgram() {
    unsigned int vertexShader = createVertexShader();
    unsigned int fragmentShader = createFragmentShader();

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

