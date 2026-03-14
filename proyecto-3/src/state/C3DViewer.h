#pragma once

#include <vector>
#include <utility>
#include <iostream>
#include <cmath>
#include <thread>
#include <atomic>
#include <mutex>

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
#include "UI.h"
#include "../utils/Camera.h"
#include "../utils/FPSCounter.h"
#include "../utils/Utils.h"
#include "../utils/Shaders.h"
#include "../objects/Skybox.h"

using namespace Shaders;

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader.h"

using namespace std;

class State;

class C3DViewer {
protected:
    State* appState = nullptr;
    Skybox* skybox = nullptr;
    FPSCounter* fpsCounter = nullptr;
    GLFWwindow* window = nullptr;
    GLuint shaderProgram = 0;
    GLuint skyboxShaderProgram = 0;

    int width = 720;
    int height = 480;
    bool firstMouse = false;
    UIMovementMode movementMode = UIMovementMode::FPS;
    pair<double,double> mousePos = {0.0,0.0};

    bool isFalling = false;
    float verticalVelocity = 0.0f;
    const float gravity = -9.8f;
    const float targetY = -1.5f;
    glm::vec3 initialCameraPos;

    // Async Loading State
    atomic<bool> isLoading{true};
    vector<ObjectData> loadedSceneData;
    thread loadingThread;
    mutex dataMutex;

public:
    C3DViewer() {}

    ~C3DViewer()
    {
        if (loadingThread.joinable()) loadingThread.join();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        delete appState;
        delete skybox;
        delete fpsCounter;

        if (shaderProgram) glDeleteProgram(shaderProgram);
        if (skyboxShaderProgram) glDeleteProgram(skyboxShaderProgram);
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

        window = glfwCreateWindow(width, height, "PROYECTO - 3", NULL, NULL);
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
        setupSkyboxShader();

        appState = new State();
        skybox = new Skybox("assets/cubemap/snow.png");
        fpsCounter = new FPSCounter();
        glViewport(0, 0, width, height);

        // Start loading scene in a background thread
        loadingThread = thread([this]() {
            try {
                auto data = FileLoader::loadScene("assets/scene/scene.obj");
                lock_guard<mutex> lock(dataMutex);
                loadedSceneData = std::move(data);
                isLoading = false;
            } catch (const exception& e) {
                cerr << "Error loading scene: " << e.what() << endl;
                isLoading = false;
            }
        });

        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorPosCallback);

        return true;
    }

    void mouseCameraMovement(double deltaTime) {
        if (isLoading || glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
            return;

        float speed = (float)(5.0 * deltaTime);
        float rotSpeed = (float)(10.0 * deltaTime);

        bool ignoreY = (movementMode == UIMovementMode::FPS);

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            Camera::getInstance().processKeyboard(FORWARD, speed, ignoreY);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            Camera::getInstance().processKeyboard(BACKWARD, speed, ignoreY);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            Camera::getInstance().processMouseMovement(-rotSpeed, 0, false);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            Camera::getInstance().processMouseMovement(rotSpeed, 0, false);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            Camera::getInstance().processKeyboard(FORWARD, speed, ignoreY);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            Camera::getInstance().processKeyboard(BACKWARD, speed, ignoreY);
    }

    void mainLoop()
    {
        double lastFrameTime = glfwGetTime();
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            double currentFrameTime = glfwGetTime();
            double deltaTime = currentFrameTime - lastFrameTime;
            lastFrameTime = currentFrameTime;

            if (!isLoading && !loadedSceneData.empty()) {
                lock_guard<mutex> lock(dataMutex);
                if (!loadedSceneData.empty()) {
                    appState->createFromData(loadedSceneData);
                    loadedSceneData.clear();
                    initialCameraPos = glm::vec3(0.0f, targetY, -2.6f);
                    Camera::getInstance().position = initialCameraPos;
                }
            }

            fpsCounter->tick(currentFrameTime);
            char title[64];
            sprintf(title, "PROYECTO - 3 | FPS: %.1f", fpsCounter->get());
            glfwSetWindowTitle(window, title);

            if (!isLoading && isFalling) {
                verticalVelocity += gravity * (float)deltaTime;
                Camera::getInstance().position.y += verticalVelocity * (float)deltaTime;
                if (Camera::getInstance().position.y <= targetY) {
                    Camera::getInstance().position.y = targetY;
                    isFalling = false;
                    verticalVelocity = 0.0f;
                }
            }

            mouseCameraMovement(deltaTime);
            glEnable(GL_DEPTH_TEST);
            render();

            glfwSwapBuffers(window);
        }
    }

private:
    void onKey(int key, int scancode, int action, int mods) 
    {
        if (action == GLFW_PRESS) 
        {
            if (key == GLFW_KEY_ESCAPE) {
                if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                } else {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
            }

            if (key == GLFW_KEY_ENTER) {
                if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
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
        if (io.WantCaptureMouse) return;

        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (action == GLFW_PRESS)
                glfwGetCursorPos(window, &mousePos.first, &mousePos.second);
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            if (action == GLFW_PRESS)
                glfwGetCursorPos(window, &mousePos.first, &mousePos.second);
        }
    }

    void onCursorPos(double xpos, double ypos) {
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
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
            mousePos = {xpos, ypos};
            return;
        }

        mousePos = {xpos,ypos};
    }

    void render()
    {
        prepareRendering(shaderProgram, 0.1f, 0.1f, 0.1f);

        if (skybox) {
            skybox->draw(skyboxShaderProgram);
        }

        glUseProgram(shaderProgram);
        if (appState && !isLoading)
        {
            DrawConfig config;
            config.shaderProgram = shaderProgram;
            config.currentTime = glfwGetTime();
            if (skybox) config.skyboxTextureID = skybox->textureID;

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

        if (isLoading) {
            UI::renderLoadingScreen(width, height);
        } else {
            UISceneContext ctx = { 
                width, height, movementMode, isFalling, verticalVelocity, 
                targetY, initialCameraPos, window, firstMouse 
            };
            UI::renderSettings(appState, ctx);
        }

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

    GLuint setupShader(const char* src, GLenum type) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        return shader;
    }

    void setupDefaultShader() {
        GLuint vs = setupShader(vertexShaderSrc, GL_VERTEX_SHADER);
        GLuint fs = setupShader(fragmentShaderSrc, GL_FRAGMENT_SHADER);
        shaderProgram = setupShaderProgram(vs, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    void setupSkyboxShader() {
        GLuint vs = setupShader(skyboxVertexShaderSrc, GL_VERTEX_SHADER);
        GLuint fs = setupShader(skyboxFragmentShaderSrc, GL_FRAGMENT_SHADER);
        skyboxShaderProgram = setupShaderProgram(vs, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    void clear(float r, float g, float b){
        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    GLuint setupShaderProgram(GLuint vs, GLuint fs) {
        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
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
