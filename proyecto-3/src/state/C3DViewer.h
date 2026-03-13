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
#include "../objects/Skybox.h"

using namespace Shaders;

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader.h"

using namespace std;

class State;

enum MovementMode {
    FPS_MODE = 0,
    GOD_MODE = 1
};

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
    MovementMode movementMode = GOD_MODE;
    pair<double,double> mousePos = {0.0,0.0};

    bool isFalling = false;
    float verticalVelocity = 0.0f;
    const float gravity = -9.8f;
    const float targetY = -1.5f;
    glm::vec3 initialCameraPos;

public:
    C3DViewer() {}

    ~C3DViewer()
    {
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

        clear(0.1f, 0.1f, 0.1f);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(width / 2.0f - 120.0f, height / 2.0f - 50.0f));
        ImGui::SetNextWindowSize(ImVec2(240, 100));
        ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("...Loading...");
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        skybox = new Skybox("assets/cubemap/snow.png");
        
        try {
            appState->loadScene("assets/scene/scene.obj");
            initialCameraPos = glm::vec3(0.0f, 0.6f, -2.6f);
            Camera::getInstance().position = initialCameraPos;
        } catch (const exception& e) {
            cerr << "Error loading scene: " << e.what() << endl;
        }

        fpsCounter = new FPSCounter();

        glViewport(0, 0, width, height);


        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorPosCallback);

        return true;
    }

    void mouseCameraMovement(double deltaTime) {
        if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
            return;

        float speed = (float)(5.0 * deltaTime);
        float rotSpeed = (float)(10.0 * deltaTime);

        bool ignoreY = (movementMode == FPS_MODE);

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

            fpsCounter->tick(currentFrameTime);
            char title[64];
            sprintf(title, "PROYECTO - 3 | FPS: %.1f", fpsCounter->get());
            glfwSetWindowTitle(window, title);

            if (isFalling) {
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
        if (appState)
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

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(300, height));
        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        if (appState) {
            if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
                for (int i = 0; i < (int)appState->lights.size(); i++) {
                    string label = "Light " + to_string(i + 1);
                    if (ImGui::TreeNode(label.c_str())) {
                        Light* l = appState->lights[i];
                        ImGui::Checkbox("Enabled", &l->enabled);
                        ImGui::SliderFloat("Intensity", &l->intensity, 0.0f, 5.0f);
                        ImGui::ColorEdit3("Diffuse",  &l->diffuse.x);
                        ImGui::ColorEdit3("Ambient",  &l->ambient.x);
                        ImGui::ColorEdit3("Specular", &l->specular.x);
                        ImGui::SliderFloat("Anim Speed", &l->animationSpeed, 0.0f, 5.0f);
                        const char* modes[] = { "Phong", "Blinn-Phong", "Flat" };
                        int currentMode = (int)l->shadingMode;
                        if (ImGui::Combo("Shading", &currentMode, modes, IM_ARRAYSIZE(modes)))
                            l->shadingMode = (ShadingMode)currentMode;
                        ImGui::TreePop();
                    }
                }
            }

            if (ImGui::CollapsingHeader("Movement")) {
                const char* moveModes[] = { "FPS", "GOD" };
                int currentMoveMode = (int)movementMode;
                if (ImGui::Combo("Mode", &currentMoveMode, moveModes, IM_ARRAYSIZE(moveModes))) {
                    MovementMode newMode = (MovementMode)currentMoveMode;
                    if (movementMode == GOD_MODE && newMode == FPS_MODE) {
                        float curY = Camera::getInstance().position.y;
                        if (curY > targetY) {
                            isFalling = true;
                            verticalVelocity = 0.0f;
                        } else if (curY < targetY - 0.01f) {
                            Camera::getInstance().position = initialCameraPos;
                        }
                    }
                    movementMode = newMode;
                }
                if (ImGui::Button("Mouse Camera")) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    firstMouse = true;
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Press ESC to release");
            }

            if (ImGui::CollapsingHeader("Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::BeginChild("obj_list", ImVec2(-1, 200), false);
                for (auto obj : appState->objects) {
                    bool sel = obj->isSelected;
                    if (ImGui::Selectable(obj->name.c_str(), sel)) {
                        for (auto o : appState->objects) o->isSelected = false;
                        if (!sel) obj->isSelected = true;
                    }
                }
                ImGui::EndChild();
            }

            Object* selected = nullptr;
            for (auto obj : appState->objects)
                if (obj->isSelected) { selected = obj; break; }

            if (selected) {
                string editLabel = "Edit - " + selected->name;
                if (ImGui::CollapsingHeader(editLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::Button("Set Diffuse Map", ImVec2(-1, 0))) {
                        auto res = pfd::open_file("Select Diffuse Map", ".", { "Image Files", "*.png *.jpg *.jpeg *.bmp *.tga" }).result();
                        if (!res.empty()) {
                            GLuint tex = FileLoader::loadTexture(res[0]);
                            if (tex) for (auto sm : selected->submeshes) sm->diffuseMap = tex;
                        }
                    }
                    if (ImGui::Button("Set Bump Map", ImVec2(-1, 0))) {
                        auto res = pfd::open_file("Select Bump Map", ".", { "Image Files", "*.png *.jpg *.jpeg *.bmp *.tga" }).result();
                        if (!res.empty()) {
                            GLuint tex = FileLoader::loadTexture(res[0]);
                            if (tex) for (auto sm : selected->submeshes) sm->normalMap = tex;
                        }
                    }
                    ImGui::Separator();
                    const char* sMappings[] = { "Standard", "Spherical", "Squared" };
                    const char* oMappings[] = { "Position", "Normal" };
                    int currentS = selected->submeshes.empty() ? 0 : selected->submeshes[0]->sMappingMode;
                    if (ImGui::Combo("s-mapping", &currentS, sMappings, IM_ARRAYSIZE(sMappings)))
                        for (auto sm : selected->submeshes) sm->sMappingMode = currentS;
                    int currentO = selected->submeshes.empty() ? 0 : selected->submeshes[0]->oMappingMode;
                    if (ImGui::Combo("o-mapping", &currentO, oMappings, IM_ARRAYSIZE(oMappings)))
                        for (auto sm : selected->submeshes) sm->oMappingMode = currentO;
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
