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
#include "../utils/FPSCounter.h"
#include "../utils/Utils.h"
#include "../utils/Shaders.h"

using namespace Shaders;

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader.h"

using namespace std;

class State;

class C3DViewer {
protected:
    State* appState = nullptr;
    FPSAvgCounter* fpsCounter = nullptr;
    GLFWwindow* window = nullptr;
    GLuint shaderProgram = 0;

    int width = 720;
    int height = 480;
    bool firstMouse = false;
    bool isFPSMode = false;
    bool showFramesSecond = true;
    bool mouseButtonsDown[2] = { false, false };
    pair<double,double> mousePos = {0.0,0.0};

    char fpsText[16] = "FPS: n/a"; // New

public:
    C3DViewer() {}

    ~C3DViewer()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        delete appState;
        delete fpsCounter;

        if (shaderProgram) glDeleteProgram(shaderProgram);
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

        appState = new State();
        
        try {
            appState->loadObject("assets/cool-table/cool-table.obj");
            // Set initial camera on top of the table edge
            Camera::getInstance().position = glm::vec3(0.0f, 0.6f, -2.6f);
        } catch (const exception& e) {
            cerr << "Error loading default object: " << e.what() << endl;
        }

        // avg of last 5 seconds
        fpsCounter = new FPSAvgCounter(5);

        glViewport(0, 0, width, height);


        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorPosCallback);

        return true;
    }

    void mouseCameraMovement(double deltaTime) {
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
            fpsCounter->framesPerSecondAvg(glfwGetTime());
            sprintf(fpsText, "FPS: %.1f",fpsCounter->getCount()); 

            mouseCameraMovement(fpsCounter->getDelta(glfwGetTime()));
            glfwPollEvents();

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
                Camera::getInstance().processMouseMovement(-10, 0, false);
            if (key == GLFW_KEY_RIGHT)
                Camera::getInstance().processMouseMovement(10, 0, false);

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
                glfwGetCursorPos(window, &mousePos.first, &mousePos.second);
            }
            else if (action == GLFW_RELEASE)
            {
                mouseButtonsDown[0] = false;
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)
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
            handleFullObjectTranslation(deltaX, -deltaY);
        }

        mousePos = {xpos,ypos};
    }

    void render() 
    {
        prepareRendering(shaderProgram, appState->backgroundColor[0], appState->backgroundColor[1], appState->backgroundColor[2]); // Use appState background color

        if (appState)
        {
            DrawConfig config;
            config.shaderProgram = shaderProgram;

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
                                Camera::getInstance().resetCamera();
                            } catch (const exception& e) {
                                cerr << "Error loading object: " << e.what() << endl;
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
            if (ImGui::CollapsingHeader("Basic", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::ColorEdit3("Background", appState->backgroundColor, ImGuiColorEditFlags_NoInputs);
                
                // FPS Display
                ImGui::Checkbox("Show##FPS", &showFramesSecond);
                if (showFramesSecond)
                    ImGui::TextUnformatted(fpsText); // Display FPS
            }

            // Advanced Options Section
            if (ImGui::CollapsingHeader("Advanced"))
            {
                ImGui::Checkbox("Back-face Culling", &appState->enableBackfaceCulling); 
                ImGui::Checkbox("Depth Test", &appState->enableDepthTest); 
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
    }

    void handleRotation(double deltaX, double deltaY){
        if (!appState) return;

        float sensitivity = 0.2f; 
        float rotationAmountY = deltaX * sensitivity; 
        float rotationAmountX = deltaY * sensitivity;

        appState->rotateObject(rotationAmountX, rotationAmountY);
    }

    void handleFullObjectTranslation(double deltaX, double deltaY) {
        if (!appState || appState->shapes.empty()) return;

        float distance = glm::distance(Camera::getInstance().position, Camera::getInstance().initObjectPos); 
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

    void clear(float r, float g, float b){
        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
