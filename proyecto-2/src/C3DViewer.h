#pragma once

#include <vector>
#include <utility>
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "portable-file-dialogs.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "State.h"
#include "Camera.h"
#include "GLUtils.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader.h"

using namespace std;

class State;

class C3DViewer 
{

public:
    C3DViewer() {}

    bool setup()
    {
        if (!glfwInit()) 
            return false;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(width, height, "C3DViewer Window: Hello Triangle", NULL, NULL);
        if (!window) 
        {
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
        {
            glfwDestroyWindow(window);
            glfwTerminate();
            return false;
        }
        
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_PROGRAM_POINT_SIZE);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
        
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int w, int h) {
            auto ptr = reinterpret_cast<C3DViewer*>(glfwGetWindowUserPointer(window));
            if (ptr) 
                ptr->resize(w, h);
        });

        setupDefaultShader();
        setupPickingShader();
        setupPickingFBO(); 

        appState = new State();

        glViewport(0, 0, width, height);

        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorPosCallback);

        return true;
    }

    void mainLoop() 
    {
        while (!glfwWindowShouldClose(window)) 
        {
            glfwPollEvents();

            if (appState && appState->lineAntialiasing) {
                glEnable(GL_LINE_SMOOTH);
                glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
            } else {
                glDisable(GL_LINE_SMOOTH);
            }

            clear(0.3f);
            render();

            glfwSwapBuffers(window);
        }
    }

    ~C3DViewer()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        delete appState;

        if (shaderProgram) glDeleteProgram(shaderProgram);
        if (pickingShaderProgram) glDeleteProgram(pickingShaderProgram);
        if (pickingTexture) glDeleteTextures(1, &pickingTexture);
        if (pickingDepthStencilRBO) glDeleteRenderbuffers(1, &pickingDepthStencilRBO);
        if (pickingFBO) glDeleteFramebuffers(1, &pickingFBO);
        if (window) glfwDestroyWindow(window);
        glfwTerminate();
    }

private:
    void onKey(int key, int scancode, int action, int mods) 
    {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) // Handle both press and hold
        {
            if (key == GLFW_KEY_ESCAPE)
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            if (key == GLFW_KEY_UP)
                camera.processKeyboard(FORWARD, 0.1f);
            if (key == GLFW_KEY_DOWN)
                camera.processKeyboard(BACKWARD, 0.1f);
            if (key == GLFW_KEY_LEFT)
                camera.processKeyboard(LEFT, 0.1f);
            if (key == GLFW_KEY_RIGHT)
                camera.processKeyboard(RIGHT, 0.1f);
        }
    }

    void onMouseButton(int button, int action, int mods) 
    {
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            return;
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (action == GLFW_PRESS)
            {
                mouseButtonsDown[0] = true;
                glfwGetCursorPos(window, &mousePos.first, &mousePos.second); // Get current mouse pos when button pressed

                glm::vec3 pickedColor = readPixelColor(mousePos.first, mousePos.second);

                int pickedID = static_cast<int>(pickedColor.x) + static_cast<int>(pickedColor.y) * 256 + static_cast<int>(pickedColor.z) * 256 * 256;

                if (selectedSubmeshIndex != -1 && selectedSubmeshIndex < appState->shapes.size()) {
                    appState->shapes[selectedSubmeshIndex]->showBoundingBox = false;
                }

                if (pickedID > 0 && appState && pickedID <= appState->shapes.size())
                {
                    selectedSubmeshIndex = pickedID - 1; // Adjust for +1 offset in shader
                    appState->shapes[selectedSubmeshIndex]->showBoundingBox = true;
                    std::cout << "Selected submesh index: " << selectedSubmeshIndex << std::endl;
                }
                else
                {
                    selectedSubmeshIndex = -1; // No submesh selected
                    std::cout << "No submesh selected." << std::endl;
                }
            }
            else if (action == GLFW_RELEASE)
            {
                mouseButtonsDown[0] = false;
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT) // Track right mouse button for rotation
        {
            if (action == GLFW_PRESS)
            {
                mouseButtonsDown[1] = true;
                glfwGetCursorPos(window, &mousePos.first, &mousePos.second); 
            }
            else if (action == GLFW_RELEASE)
            {
                mouseButtonsDown[1] = false;
            }
        }
    }

    void onCursorPos(double xpos, double ypos) 
    {
        ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            mousePos = {xpos, ypos}; // Update position to prevent jumps
            return;
        }

        double deltaX = xpos - mousePos.first;
        double deltaY = ypos - mousePos.second;

        if (mouseButtonsDown[1]){
            handleRotation(deltaX, deltaY);
        } else if (mouseButtonsDown[0]){
            handleTranslation(deltaX, -deltaY, selectedSubmeshIndex);
        }
        
        mousePos = {xpos,ypos};
    }

    void render() 
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
        prepareRendering(pickingShaderProgram, 0.0f);

        if (appState)
        {
            appState->drawPicking(pickingShaderProgram);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind default framebuffer
        
        prepareRendering(shaderProgram, 0.1f);

        if (appState)
        {
            appState->draw(shaderProgram, selectedSubmeshIndex);
        }

        drawInterface();
    }

    void prepareRendering(GLuint program, float clearColor)
    {
        clear(clearColor);
        glEnable(GL_DEPTH_TEST);

        glUseProgram(program);

        glm::mat4 view = camera.getViewMatrix();
        setGpuVariable(program, "view", view);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
        setGpuVariable(program, "projection", projection);
    }

    void drawInterface()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Once);
        ImGui::Begin("Control Panel");
        
        if (ImGui::Button("Load Model"))
        {
            auto selection = pfd::open_file("Choose OBJ file", ".", { "OBJ Files (.obj)", "*.obj" }).result();
            if (!selection.empty())
            {
                if (appState) {
                    try {
                        appState->loadObject(selection[0]);
                    } catch (const exception& e) {
                        cerr << "Error loading object: " << e.what() << endl;
                    }
                }
            }
        }

        static int scaleValue = 50; // Initial value for 1.0 scale
        if (ImGui::SliderInt("Scale", &scaleValue, 10, 200)) 
        {
            if (appState) {
                float scaleFactor = static_cast<float>(scaleValue) / 50.0f; // Map 50 to 1.0, 1 to 0.02, 100 to 2.0
                appState->rescaleAllShapes(scaleFactor);
            }
        }

        if (appState) {
            ImGui::Checkbox("Show Vertices", &appState->showVertices);
            ImGui::ColorEdit3("Vertex Color", appState->vertexColor);
            ImGui::SliderFloat("Point Size", &appState->pointSize, 1.0f, 20.0f);
            ImGui::Checkbox("Show Fill", &appState->showFill);
            ImGui::Checkbox("Show Wireframe", &appState->showWireframe);
            ImGui::ColorEdit3("Wireframe Color", appState->wireframeColor);
            ImGui::Checkbox("Line Antialiasing", &appState->lineAntialiasing);
        }

        ImGui::End();

        if (selectedSubmeshIndex != -1 && appState && selectedSubmeshIndex < appState->shapes.size())
        {
            ImGui::Begin("Selected Submesh Controls");

            Submesh* selectedSubmesh = appState->shapes[selectedSubmeshIndex];

            if (ImGui::Button("Delete Submesh"))
            {
                appState->deleteSubmesh(selectedSubmeshIndex);
                selectedSubmeshIndex = -1; 
            }
            else
            {
                ImGui::ColorEdit3("Bounding Box Color", selectedSubmesh->boundingBoxColor);
            }
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void resize(int newWidth, int newHeight) 
    {
        width = newWidth;
        height = newHeight;
        glViewport(0, 0, width, height);
    }

    void handleRotation(double deltaX, double deltaY){
        if (!appState) return;

        float sensitivity = 0.2f; 
        float rotationAmountY = deltaX * sensitivity; 
        float rotationAmountX = deltaY * sensitivity;

        appState->rotateObject(rotationAmountX, rotationAmountY);
    }

    void handleTranslation(double deltaX, double deltaY, int submeshIndex){
        if (!appState || appState->shapes.empty() || submeshIndex < 0) return;

        glm::vec3 objectWorldPos = glm::vec3(appState->shapes[submeshIndex]->getTransform()[3]);
        float distance = glm::distance(camera.position, objectWorldPos);

        float sensitivity = 0.03f; 
        float moveFactor = (1.0f + distance) * sensitivity;

        glm::vec3 translationVector = (camera.right * (float)deltaX * moveFactor) + (camera.up * (float)deltaY * moveFactor);

        appState->translateSubmesh(submeshIndex, translationVector);
    }

    GLuint setupShader(const char* name, const char* src, const GLenum type) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        if (!checkCompileErrors(shader, name)) 
            throw invalid_argument("set: out of bounds");

        return shader;
    }

    void setupDefaultShader() 
    {
        GLuint vertexShader = setupShader("VERTEX", vertexShaderSrc, GL_VERTEX_SHADER);
        GLuint fragmentShader = setupShader("FRAGMENT", fragmentShaderSrc,GL_FRAGMENT_SHADER);

        shaderProgram = setupShaderProgram(vertexShader,fragmentShader, "PROGRAM");

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    bool setupPickingFBO()
    {
        // Generate FBO
        glGenFramebuffers(1, &pickingFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);

        // Create a texture for rendering the object IDs
        glGenTextures(1, &pickingTexture);
        glBindTexture(GL_TEXTURE_2D, pickingTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickingTexture, 0);

        // Create a renderbuffer object for depth and stencil attachment
        glGenRenderbuffers(1, &pickingDepthStencilRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, pickingDepthStencilRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, pickingDepthStencilRBO);

        // Check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "ERROR::FRAMEBUFFER:: Picking Framebuffer is not complete!\n");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind FBO
        return true;
    }

    GLuint setupShaderProgram(GLuint vertexShader, GLuint fragmentShader, const char* name){
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        if (!checkCompileErrors(program, name)) 
            throw runtime_error("error compiling shader program");

        return program;
    }

    void setupPickingShader()
    {
        GLuint vertexShader = setupShader("PICKING_VERTEX",pickingVertexShaderSrc, GL_VERTEX_SHADER);
        GLuint fragmentShader = setupShader("PICKING_FRAGMENT",pickingFragmentShaderSrc, GL_FRAGMENT_SHADER);

        pickingShaderProgram = setupShaderProgram(vertexShader,fragmentShader, "PICKING_PROGRAM");

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    bool checkCompileErrors(GLuint shader, const char* type) 
    {
        GLint success;
        GLchar infoLog[1024];
        if (strcmp(type, "PROGRAM") != 0) 
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) 
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                fprintf(stderr, "ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s\n", type, infoLog);
                return false;
            }
        }
        else 
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                fprintf(stderr, "ERROR::PROGRAM_LINKING_ERROR of type: %s\n%s\n", type, infoLog);
                return false;
            }
        }
        return true;
    }

    glm::vec3 readPixelColor(double xpos, double ypos)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
        unsigned char data[3]; // RGB
        glReadPixels(static_cast<int>(xpos), height - 1 - static_cast<int>(ypos), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return glm::vec3(data[0], data[1], data[2]);
    }

    void clear(float color){
        glClearColor(color, color, color, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) 
    {
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        C3DViewer* self = (C3DViewer*)glfwGetWindowUserPointer(window);
        if (self) self->onKey(key, scancode, action, mods);
    }

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) 
    {
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
        C3DViewer* self = (C3DViewer*)glfwGetWindowUserPointer(window);
        if (self) self->onMouseButton(button, action, mods);
    }

    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) 
    {
        ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
        C3DViewer* self = (C3DViewer*)glfwGetWindowUserPointer(window);
        if (self) self->onCursorPos(xpos, ypos);
    }


protected:
    GLFWwindow* window = nullptr;
    State* appState = nullptr;
    GLuint shaderProgram = 0;
    Camera camera;

    int width = 720;
    int height = 480;
    bool mouseButtonsDown[2] = { false, false };
    pair<double,double> mousePos = {0.0,0.0};
    int selectedSubmeshIndex = -1; // -1 means no submesh is selected

    // Picking FBO and related textures/renderbuffers
    GLuint pickingFBO = 0;
    GLuint pickingTexture = 0;
    GLuint pickingDepthStencilRBO = 0;
    GLuint pickingShaderProgram = 0;

    const char* vertexShaderSrc = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec3 aColor;
        layout(location = 3) in vec2 aTexCoord;
        out vec3 vNormal;
        out vec3 vColor;
        out vec2 vTexCoord;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform bool u_render_points;
        uniform float u_point_size;
        void main() 
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            vNormal = aNormal;
            vColor = aColor;
            vTexCoord = aTexCoord;
            if (u_render_points) {
                gl_PointSize = u_point_size;
            }
        }
    )glsl";

    const char* fragmentShaderSrc = R"glsl(
        #version 330 core
        in vec3 vNormal;
        in vec3 vColor;
        in vec2 vTexCoord;
        out vec4 FragColor;
        uniform bool uHasTexture;
        uniform sampler2D uTexture;
        uniform bool u_render_points;
        uniform vec3 vertexColor;
        uniform bool u_is_wireframe;
        uniform vec3 u_wireframe_color;
        void main() {
            if (u_is_wireframe) {
                FragColor = vec4(u_wireframe_color, 1.0);
            } else if (u_render_points) {
                FragColor = vec4(vertexColor, 1.0);
            } else if (uHasTexture) {
                FragColor = texture(uTexture, vTexCoord);
            } else {
                FragColor = vec4(vColor, 1.0);
            }
        }
    )glsl";

    const char* pickingVertexShaderSrc = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main()
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )glsl";

    const char* pickingFragmentShaderSrc = R"glsl(
        #version 330 core
        out vec4 FragColor;
        uniform int objectId;
        void main()
        {
            // Encode objectId into color components
            FragColor = vec4(
                float((objectId >> 0) & 0xFF) / 255.0f,
                float((objectId >> 8) & 0xFF) / 255.0f,
                float((objectId >> 16) & 0xFF) / 255.0f,
                1.0f
            );
        }
    )glsl";
};
