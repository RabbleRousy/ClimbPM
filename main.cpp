#include "ProjectorConfig.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

void render(cv::Mat content);

int main()
{
    auto testImg = imread("../Resources/test-image.jpg");
    render(testImg);

    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void renderColor(float r, float g, float b, float a);
unsigned int createVertexBuffer();
unsigned int createElementBuffer();
unsigned int createVertexArray(unsigned int vertexBuffer, unsigned int elementBuffer);
unsigned int createVertexShader();
unsigned int createFragmentShader();
unsigned int createShaderProgram();

void render(cv::Mat content)
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(content.cols, content.rows, "ClimbPM", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
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
    // Flip vertically
    flip(content, content, 0);
    // Upload the image to the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, content.cols, content.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, content.ptr());

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        glUseProgram(shader);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shader);
    glfwTerminate();
}

// #################################################################################
// ########################### GLFW FUNCTIONS ######################################
// #################################################################################

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// Simplest render function, uses the given color to clear the screen
// https://learnopengl.com/Getting-started/Hello-Window
void renderColor(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

unsigned int createVertexBuffer() {
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

unsigned int createElementBuffer() {
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

unsigned int createVertexArray(unsigned int vertexBuffer, unsigned int elementBuffer) {
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

unsigned int createVertexShader() {
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

unsigned int createFragmentShader() {
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

unsigned int createShaderProgram() {
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

