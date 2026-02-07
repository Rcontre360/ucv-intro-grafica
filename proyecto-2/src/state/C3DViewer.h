#pragma once

#include <vector>
#include <utility>
#include <iostream>
#include <cmath>
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

class C3DViewer {
protected:
    GLFWwindow* window = nullptr;
    State* appState = nullptr;
    GLuint shaderProgram = 0;
    GLuint normalShaderProgram = 0;

    int width = 720;
    int height = 480;
    bool mouseButtonsDown[2] = { false, false };
    bool firstMouse = false;
    bool isFPSMode = false;
    pair<double,double> mousePos = {0.0,0.0};
    int selectedSubmeshIndex = -1;

    double lastFrameTime = 0.0; // New
    int frameCount = 0; // New
    char fpsText[16] = "FPS: n/a"; // New

    GLuint pickingFBO = 0;
    GLuint pickingTexture = 0;
    GLuint pickingDepthStencilRBO = 0;
    GLuint pickingShaderProgram = 0;

public:
    C3DViewer() {}

    ~C3DViewer()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        delete appState;

        if (shaderProgram) glDeleteProgram(shaderProgram);
        if (pickingShaderProgram) glDeleteProgram(pickingShaderProgram);
        if (normalShaderProgram) glDeleteProgram(normalShaderProgram); // New: Clean up normal shader program
        if (pickingTexture) glDeleteTextures(1, &pickingTexture);
        if (pickingDepthStencilRBO) glDeleteRenderbuffers(1, &pickingDepthStencilRBO);
        if (pickingFBO) glDeleteFramebuffers(1, &pickingFBO);
        if (window) glfwDestroyWindow(window);
        glfwTerminate();
    }

    bool setup()
    {
        if (!glfwInit()) 
            return false;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);

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
        setupNormalShader(); 
        setupPickingFBO(); 

        appState = new State();

        glViewport(0, 0, width, height);


        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorPosCallback);

        return true;
    }

    void processFPS(double deltaTime) {
        float speed = (float)(1.0 * deltaTime); // Adjust speed constant as needed

        if (isFPSMode) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                Camera::getInstance().processKeyboard(FORWARD, speed);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                Camera::getInstance().processKeyboard(BACKWARD, speed);
        }
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

            processFPS(deltaTime);
            glfwPollEvents();

            if (appState && appState->lineAntialiasing) {
                glEnable(GL_MULTISAMPLE);
                glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
            } else {
                glDisable(GL_MULTISAMPLE);
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

private:
    void onKey(int key, int scancode, int action, int mods) 
    {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) 
        {
            if (key == GLFW_KEY_ESCAPE)
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            if (key == GLFW_KEY_UP)
                Camera::getInstance().processKeyboard(FORWARD, 1);
            if (key == GLFW_KEY_DOWN)
                Camera::getInstance().processKeyboard(BACKWARD, 1);
            if (key == GLFW_KEY_LEFT)
                Camera::getInstance().processMouseMovement(-7, 0, false);
            if (key == GLFW_KEY_RIGHT)
                Camera::getInstance().processMouseMovement(7, 0, false);

            if (key == GLFW_KEY_SPACE) {
                isFPSMode = !isFPSMode;
                if (isFPSMode) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    firstMouse = true; 
                } else {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
            }
        }
    }

    void onMouseButton(int button, int action, int mods) 
    {
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

    void onCursorPos(double xpos, double ypos) {
        if (isFPSMode) {
            if (firstMouse) {
                mousePos = {xpos, ypos};
                firstMouse = false;
            }

            float deltaX = (float)(xpos - mousePos.first);
            float deltaY = (float)(mousePos.second - ypos); 
            mousePos = {xpos, ypos};

            Camera::getInstance().processMouseMovement(deltaX, deltaY);
            return; 
        }

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
            DrawConfig config;
            config.shaderProgram = shaderProgram;
            config.normalShaderProgram = normalShaderProgram;
            config.showVertices = appState->showVertices;
            config.vertexColor = appState->vertexColor;
            config.pointSize = appState->pointSize;
            config.showWireframe = appState->showWireframe;
            config.wireframeColor = appState->wireframeColor;
            config.showFill = appState->showFill;
            config.showNormals = appState->showNormals;
            config.normalColor = appState->normalColor;
            config.normalWidth = appState->normalWidth;

            appState->draw(config);
        }

        drawInterface();
    }

    void prepareRendering(GLuint program, float r, float g, float b)
    {
        clear(r, g, b);
        
        glUseProgram(program);

        glm::mat4 view = Camera::getInstance().getViewMatrix();
        setGpuVariable(program, DefaultShader::view, view);

        setGpuVariable(program, DefaultShader::projection, Camera::getInstance().projection);
    }

    void drawInterface()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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
                    auto selection = pfd::save_file("Save OBJ file", ".", { "OBJ Files (.obj)", "*.obj" }).result();
                    if (!selection.empty())
                    {
                        if (appState) {
                            try {
                                FileLoader::saveObject(selection, appState->shapes);
                            } catch (const exception& e) {
                                cerr << "Error saving object: " << e.what() << endl;
                            }
                        }
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::SetNextWindowPos(ImVec2(0, 20));
        ImGui::SetNextWindowSize(ImVec2(200, height - 20));

        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar); 

        if (appState) {
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
                
                static glm::vec3 scaleValue = glm::vec3(1.0f);
                if (ImGui::SliderFloat("Scale X", &scaleValue.x, 0.01, 3))
                {
                    appState->moveFullObjectMode = false;
                    appState->rescaleAllShapes(scaleValue);
                }
                if (ImGui::SliderFloat("Scale Y", &scaleValue.y, 0.01, 3))
                {
                    appState->moveFullObjectMode = false;
                    appState->rescaleAllShapes(scaleValue);
                }
                if (ImGui::SliderFloat("Scale Z", &scaleValue.z, 0.01, 3))
                {
                    appState->moveFullObjectMode = false;
                    appState->rescaleAllShapes(scaleValue);
                }
                if (ImGui::Button("Center")) {
                    if (appState) {
                        appState->centerAndScaleBack();
                        Camera::getInstance().resetCamera();
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
                ImGui::SliderFloat("Size##Normals", &appState->normalWidth, 0.1f, 100.0f);
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

            // Selected Submesh Controls as a collapsible header
            if (ImGui::CollapsingHeader("Selected Submesh") && selectedSubmeshIndex != -1 && appState && selectedSubmeshIndex < appState->shapes.size())
            {
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
            }
        }

        ImGui::End(); 

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void resize(int newWidth, int newHeight) 
    {
        width = newWidth;
        height = newHeight;

        Camera::getInstance().setProjection(width,height);

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

        Camera &cam = Camera::getInstance();
        glm::vec3 objectWorldPos = glm::vec3(appState->shapes[submeshIndex]->getTransform()[3]);
        float distance = glm::dot(objectWorldPos - cam.position, cam.front);

        float theta = glm::radians(cam.zoom/ 2.0f);
        float halfTan = tanf(theta);

        float heightAtDepth = distance * halfTan;
        float moveFactor = (heightAtDepth * 2.0f) / height;

        glm::vec3 translationVector = (cam.right * (float)deltaX * moveFactor) + (cam.up * (float)deltaY * moveFactor);

        appState->translateSubmesh(submeshIndex, translationVector);
    }

    void handleFullObjectTranslation(double deltaX, double deltaY) {
        if (!appState || appState->shapes.empty()) return;

        float distance = glm::distance(Camera::getInstance().position, Camera::getInstance().initObjectPos); // Approximate distance to object
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

        return shader;
    }

    void setupDefaultShader() 
    {
        GLuint vertexShader = setupShader("VERTEX", vertexShaderSrc, GL_VERTEX_SHADER);
        GLuint fragmentShader = setupShader("FRAGMENT", fragmentShaderSrc,GL_FRAGMENT_SHADER);

        shaderProgram = setupShaderProgram(vertexShader,fragmentShader);

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

    GLuint setupShaderProgram(GLuint vertexShader, GLuint fragmentShader, GLuint geometryShader = 0){
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        if (geometryShader) {
            glAttachShader(program, geometryShader);
        }
        glLinkProgram(program);

        return program;
    }

    void setupPickingShader()
    {
        GLuint vertexShader = setupShader("PICKING_VERTEX",pickingVertexShaderSrc, GL_VERTEX_SHADER);
        GLuint fragmentShader = setupShader("PICKING_FRAGMENT",pickingFragmentShaderSrc, GL_FRAGMENT_SHADER);

        pickingShaderProgram = setupShaderProgram(vertexShader,fragmentShader);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void setupNormalShader() 
    {
        GLuint vertexShader = setupShader("NORMAL_VERTEX", normalVertexShaderSrc, GL_VERTEX_SHADER);
        GLuint geometryShader = setupShader("NORMAL_GEOMETRY", normalGeometryShaderSrc, GL_GEOMETRY_SHADER);
        GLuint fragmentShader = setupShader("NORMAL_FRAGMENT", normalFragmentShaderSrc, GL_FRAGMENT_SHADER);

        normalShaderProgram = setupShaderProgram(vertexShader, fragmentShader, geometryShader);

        glDeleteShader(vertexShader);
        glDeleteShader(geometryShader); 
        glDeleteShader(fragmentShader);
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

};
