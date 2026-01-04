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
#include "state.h"

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

        m_window = glfwCreateWindow(width, height, "C3DViewer Window: Hello Triangle", NULL, NULL);
        if (!m_window) 
        {
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(m_window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
        {
            glfwDestroyWindow(m_window);
            glfwTerminate();
            return false;
        }
        
        glEnable(GL_DEPTH_TEST);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
        
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int w, int h) {
            auto ptr = reinterpret_cast<C3DViewer*>(glfwGetWindowUserPointer(window));
            if (ptr) 
                ptr->resize(w, h);
        });

        if (!setupShader()) return false;

        appState = new State();

        glViewport(0, 0, width, height);

        glfwSetWindowUserPointer(m_window, this);
        glfwSetKeyCallback(m_window, keyCallback);
        glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
        glfwSetCursorPosCallback(m_window, cursorPosCallback);

        return true;
    }

    void mainLoop() 
    {
        while (!glfwWindowShouldClose(m_window)) 
        {
            glfwPollEvents();

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            render();

            glfwSwapBuffers(m_window);
        }
    }

    ~C3DViewer()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        delete appState;

        if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
        if (m_window) glfwDestroyWindow(m_window);
        glfwTerminate();
    }

private:
    void onKey(int key, int scancode, int action, int mods) 
    {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) // Handle both press and hold
        {
            if (key == GLFW_KEY_ESCAPE)
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
            if (key == GLFW_KEY_UP)
                cameraPos += m_cameraSpeed * cameraFront;
            if (key == GLFW_KEY_DOWN)
                cameraPos -= m_cameraSpeed * cameraFront;
            if (key == GLFW_KEY_LEFT)
                cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * m_cameraSpeed;
            if (key == GLFW_KEY_RIGHT)
                cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * m_cameraSpeed;
        }
    }

    void onMouseButton(int button, int action, int mods) 
    {
        ImGui_ImplGlfw_MouseButtonCallback(m_window, button, action, mods);

        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (action == GLFW_PRESS)
            {
                mouseButtonsDown[0] = true;
                glfwGetCursorPos(m_window, &mousePos.first, &mousePos.second); // Get current mouse pos when button pressed
            }
            else if (action == GLFW_RELEASE)
            {
                mouseButtonsDown[0] = false;
            }
        }
    }

    void onCursorPos(double xpos, double ypos) 
    {
        ImGui_ImplGlfw_CursorPosCallback(m_window, xpos, ypos);

        if (mouseButtonsDown[0])
        {
            double deltaX = xpos - mousePos.first;
            double deltaY = ypos - mousePos.second;

            handleRotation(deltaX, deltaY);

            mousePos = {xpos,ypos};
        }
    }

    void render() 
    {
        glUseProgram(m_shaderProgram);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        GLint viewLoc = glGetUniformLocation(m_shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
        GLint projectionLoc = glGetUniformLocation(m_shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        if (appState)
        {
            appState->draw(m_shaderProgram);
        }

        drawInterface();
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
                        appState->load_object(selection[0]);
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
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void resize(int new_width, int new_height) 
    {
        width = new_width;
        height = new_height;
        glViewport(0, 0, width, height);
    }

    void handleRotation(double deltaX, double deltaY)
    {
        if (!appState) return;

        float sensitivity = 0.2f; 
        float rotationAmountY = deltaX * sensitivity; 
        float rotationAmountX = deltaY * sensitivity;

        appState->rotateObject(rotationAmountX, rotationAmountY);
    }

    bool setupShader() 
    {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
        glCompileShader(vertexShader);
        if (!checkCompileErrors(vertexShader, "VERTEX")) return false;

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
        glCompileShader(fragmentShader);
        if (!checkCompileErrors(fragmentShader, "FRAGMENT")) return false;

        m_shaderProgram = glCreateProgram();
        glAttachShader(m_shaderProgram, vertexShader);
        glAttachShader(m_shaderProgram, fragmentShader);
        glLinkProgram(m_shaderProgram);
        if (!checkCompileErrors(m_shaderProgram, "PROGRAM")) return false;

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return true;
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
    GLFWwindow* m_window = nullptr;
    State* appState = nullptr;
    GLuint m_shaderProgram = 0;

    int width = 720;
    int height = 480;
    bool mouseButtonsDown[2] = { false, false };
    pair<double,double> mousePos = {0.0,0.0};
    float m_cameraSpeed = 0.05f; // Default camera movement speed

    glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  0.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

    const char* vertexShaderSrc = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        out vec3 vColor;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() 
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            vColor = aColor;
        }
    )glsl";

    const char* fragmentShaderSrc = R"glsl(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vColor, 1.0);
        }
    )glsl";
};
