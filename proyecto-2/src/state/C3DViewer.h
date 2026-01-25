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
#include "../utils/Camera.h"
#include "../utils/Utils.h"
#include "../utils/Shaders.h"

using namespace Shaders;

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
        
        glEnable(GL_PROGRAM_POINT_SIZE);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.Fonts->AddFontFromFileTTF("include/imgui/misc/fonts/Roboto-Medium.ttf", 18.0f);

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
            double currentFrameTime = glfwGetTime();
            double deltaTime = currentFrameTime - lastFrameTime; 

            frameCount++;
            if (deltaTime >= 1.0) { 
                sprintf(fpsText, "FPS: %.1f",(float)frameCount / (float)deltaTime); 
                frameCount = 0;
                lastFrameTime = currentFrameTime; 
            }

            glfwPollEvents();

            if (appState && appState->lineAntialiasing) {
                glEnable(GL_LINE_SMOOTH);
                glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
            } else {
                glDisable(GL_LINE_SMOOTH);
            }

            if (appState && appState->enableBackfaceCulling) {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            } else {
                glDisable(GL_CULL_FACE);
            }

            if (appState && appState->enableDepthTest) {
                glEnable(GL_DEPTH_TEST);
            } else {
                glDisable(GL_DEPTH_TEST);
            }

            clear(appState->backgroundColor[0], appState->backgroundColor[1], appState->backgroundColor[2]); // Modified clear call
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
                Camera::getInstance().processKeyboard(FORWARD, 0.1f);
            if (key == GLFW_KEY_DOWN)
                Camera::getInstance().processKeyboard(BACKWARD, 0.1f);
            if (key == GLFW_KEY_LEFT)
                Camera::getInstance().processKeyboard(LEFT, 0.1f);
            if (key == GLFW_KEY_RIGHT)
                Camera::getInstance().processKeyboard(RIGHT, 0.1f);
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

                if (appState && !appState->moveFullObjectMode) {
                    glm::vec3 pickedColor = readPixelColor(mousePos.first, mousePos.second);

                    int pickedID = static_cast<int>(pickedColor.x) + static_cast<int>(pickedColor.y) * 256 + static_cast<int>(pickedColor.z) * 256 * 256;

                    if (pickedID > 0 && appState){
                        appState->setSelected(selectedSubmeshIndex, false);
                        appState->setSelected(pickedID - 1, true);
                        selectedSubmeshIndex = pickedID - 1;
                    }
                    else
                    {
                        selectedSubmeshIndex = -1; 
                    }
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
            if (appState && appState->moveFullObjectMode) {
                handleFullObjectTranslation(deltaX, -deltaY);
            } else {
                handleTranslation(deltaX, -deltaY, selectedSubmeshIndex);
            }
        }
        
        mousePos = {xpos,ypos};
    }

    void render() 
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
        prepareRendering(pickingShaderProgram, 0.0f, 0.0f, 0.0f); // Use black for picking FBO

        if (appState)
        {
            appState->drawPicking(pickingShaderProgram);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        prepareRendering(shaderProgram, appState->backgroundColor[0], appState->backgroundColor[1], appState->backgroundColor[2]); // Use appState background color

        if (appState)
        {
            appState->draw(shaderProgram);
        }

        drawInterface();
    }

    void prepareRendering(GLuint program, float r, float g, float b)
    {
        clear(r, g, b);
        
        glUseProgram(program);

        glm::mat4 view = Camera::getInstance().getViewMatrix();
        setGpuVariable(program, DefaultShader::view, view);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
        setGpuVariable(program, DefaultShader::projection, projection);
    }

    void drawInterface()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Top menu bar
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Load"))
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
                if (ImGui::MenuItem("Save"))
                {
                    // Not implemented yet
                }
                if (ImGui::MenuItem("Reset Object"))
                {
                    if (appState) {
                        appState->resetObject();
                        selectedSubmeshIndex = -1;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Left-side panel
        ImGui::SetNextWindowPos(ImVec2(0, 20));
        ImGui::SetNextWindowSize(ImVec2(200, height - 20)); // Increased width for better layout

        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar); // Added NoTitleBar for cleaner look

        if (appState) {
            // Basic Options Section
            if (ImGui::CollapsingHeader("Basic"))
            {
                ImGui::Checkbox("Show fill", &appState->showFill);
                if (ImGui::Checkbox("Move full object", &appState->moveFullObjectMode)) {
                    if (appState->moveFullObjectMode) {
                        appState->updateGlobalBoundingBox();
                        appState->unselectAll();
                    }
                }
                ImGui::ColorEdit3("Background", appState->backgroundColor, ImGuiColorEditFlags_NoInputs); // New
                
                static int scaleValue = 100;
                if (ImGui::SliderInt("Scale", &scaleValue, 10, 500))
                {
                    if (appState) {
                        float scaleFactor = static_cast<float>(scaleValue) / 100.0f;
                        appState->rescaleAllShapes(scaleFactor);
                    }
                }
            }

            // Other Sections (Vertex, Wireframe, Normals, More)
            if (ImGui::CollapsingHeader("Vertex"))
            {
                ImGui::Checkbox("Show##Vertex", &appState->showVertices);
                ImGui::SliderFloat("Size##Vertex", &appState->pointSize, 1.0f, 20.0f);
                ImGui::ColorEdit3("Color##Vertex", appState->vertexColor, ImGuiColorEditFlags_NoInputs);
            }

            if (ImGui::CollapsingHeader("Wireframe"))
            {
                ImGui::Checkbox("Show##Wireframe", &appState->showWireframe);
                ImGui::ColorEdit3("Color##Wireframe", appState->wireframeColor, ImGuiColorEditFlags_NoInputs);
            }

            if (ImGui::CollapsingHeader("Normals"))
            {
                ImGui::Checkbox("Show##Normals", &appState->showNormals);
                ImGui::ColorEdit3("Color##Normals", appState->normalColor, ImGuiColorEditFlags_NoInputs);
            }

            // Advanced Options Section
            if (ImGui::CollapsingHeader("Advanced"))
            {
                ImGui::Checkbox("Antialiasing", &appState->lineAntialiasing);
                ImGui::Checkbox("Back-face Culling", &appState->enableBackfaceCulling); // New
                ImGui::Checkbox("Depth Test", &appState->enableDepthTest); // New
            }

            // FPS Display
            ImGui::TextUnformatted(fpsText); // Display FPS
        }

        ImGui::End(); // End Settings window

        // Selected Submesh Controls window
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
                if (ImGui::ColorEdit3("Color", selectedSubmesh->color, ImGuiColorEditFlags_NoInputs)) {
                    selectedSubmesh->updateColor();
                }
                ImGui::ColorEdit3("Bounding Box Color", selectedSubmesh->boundingBoxColor, ImGuiColorEditFlags_NoInputs);
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

        glBindTexture(GL_TEXTURE_2D, pickingTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

        glBindRenderbuffer(GL_RENDERBUFFER, pickingDepthStencilRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
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
        float distance = glm::distance(Camera::getInstance().position, Camera::getInstance().initialObjectPosition); 

        float theta = glm::radians(45.0f / 2.0f);
        float halfTan = tanf(theta);

        float heightAtDepth = distance * halfTan;
        float moveFactor = (heightAtDepth * 2.0f) / height;

        glm::vec3 translationVector = (Camera::getInstance().right * (float)deltaX * moveFactor) + (Camera::getInstance().up * (float)deltaY * moveFactor);

        appState->translateSubmesh(submeshIndex, translationVector);
    }

    void handleFullObjectTranslation(double deltaX, double deltaY) {
        if (!appState || appState->shapes.empty()) return;

        float distance = glm::distance(Camera::getInstance().position, Camera::getInstance().initialObjectPosition); // Approximate distance to object
                                                                                       //
        float theta = glm::radians(45.0f / 2.0f);
        float halfTan = tanf(theta);

        float heightAtDepth = distance * halfTan;
        float moveFactor = (heightAtDepth * 2.0f) / height;

        glm::vec3 translationVector = (Camera::getInstance().right * (float)deltaX * moveFactor) + (Camera::getInstance().up * (float)deltaY * moveFactor);

        appState->translateFullObject(translationVector);
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
        glGenFramebuffers(1, &pickingFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);

        glGenTextures(1, &pickingTexture);
        glBindTexture(GL_TEXTURE_2D, pickingTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickingTexture, 0);

        glGenRenderbuffers(1, &pickingDepthStencilRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, pickingDepthStencilRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, pickingDepthStencilRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "ERROR::FRAMEBUFFER:: Picking Framebuffer is not complete!\n");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    GLuint setupShaderProgram(GLuint vertexShader, GLuint fragmentShader, const char* name, GLuint geometryShader = 0){
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        if (geometryShader) {
            glAttachShader(program, geometryShader);
        }
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
        unsigned char data[3];
        glReadPixels(static_cast<int>(xpos), height - 1 - static_cast<int>(ypos), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return glm::vec3(data[0], data[1], data[2]);
    }

    void clear(float r, float g, float b){
        glClearColor(r, g, b, 1.0f);
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

    int width = 720;
    int height = 480;
    bool mouseButtonsDown[2] = { false, false };
    pair<double,double> mousePos = {0.0,0.0};
    int selectedSubmeshIndex = -1;

    double lastFrameTime = 0.0; // New
    int frameCount = 0; // New
    char fpsText[16] = "FPS: n/a"; // New

    GLuint pickingFBO = 0;
    GLuint pickingTexture = 0;
    GLuint pickingDepthStencilRBO = 0;
    GLuint pickingShaderProgram = 0;

};
