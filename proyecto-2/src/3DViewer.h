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

        if (!setupShader()) return false;
        if (!setupPickingShader()) return false;
        if (!setupPickingFBO()) return false;

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

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
                cameraPos += cameraSpeed * cameraFront;
            if (key == GLFW_KEY_DOWN)
                cameraPos -= cameraSpeed * cameraFront;
            if (key == GLFW_KEY_LEFT)
                cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
            if (key == GLFW_KEY_RIGHT)
                cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        }
    }

    void onMouseButton(int button, int action, int mods) 
    {
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (action == GLFW_PRESS)
            {
                mouseButtonsDown[0] = true;
                glfwGetCursorPos(window, &mousePos.first, &mousePos.second); // Get current mouse pos when button pressed

                glm::vec3 pickedColor = readPixelColor(mousePos.first, mousePos.second);

                int pickedID = static_cast<int>(pickedColor.x) + static_cast<int>(pickedColor.y) * 256 + static_cast<int>(pickedColor.z) * 256 * 256;
                if (pickedID > 0 && appState && pickedID <= appState->shapes.size())
                {
                    selectedSubmeshIndex = pickedID - 1; // Adjust for +1 offset in shader
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
                glfwGetCursorPos(window, &mousePos.first, &mousePos.second); // Get current mouse pos when button pressed
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

        if (mouseButtonsDown[1]) // Check for right mouse button for rotation
        {
            double deltaX = xpos - mousePos.first;
            double deltaY = ypos - mousePos.second;

            handleRotation(deltaX, deltaY);

            mousePos = {xpos,ypos};
        }
    }

    void render() 
    {
        // --- Picking Pass ---
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Clear with black (ID 0)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        glUseProgram(pickingShaderProgram);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        GLint viewLoc = glGetUniformLocation(pickingShaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
        GLint projectionLoc = glGetUniformLocation(pickingShaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        if (appState)
        {
            for (size_t i = 0; i < appState->shapes.size(); ++i)
            {
                GLint objectIdLoc = glGetUniformLocation(pickingShaderProgram, "objectId");
                glUniform1i(objectIdLoc, i + 1); // +1 to avoid ID 0 (black)
                appState->shapes[i]->draw(pickingShaderProgram, false);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind default framebuffer
        
        // --- Regular Rendering Pass ---
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        glUseProgram(shaderProgram);

        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
        projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        if (appState)
        {
            appState->draw(shaderProgram, selectedSubmeshIndex);
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

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        if (!checkCompileErrors(shaderProgram, "PROGRAM")) return false;

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return true;
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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

    bool setupPickingShader()
    {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &pickingVertexShaderSrc, nullptr);
        glCompileShader(vertexShader);
        if (!checkCompileErrors(vertexShader, "PICKING_VERTEX")) return false;

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &pickingFragmentShaderSrc, nullptr);
        glCompileShader(fragmentShader);
        if (!checkCompileErrors(fragmentShader, "PICKING_FRAGMENT")) return false;

        pickingShaderProgram = glCreateProgram();
        glAttachShader(pickingShaderProgram, vertexShader);
        glAttachShader(pickingShaderProgram, fragmentShader);
        glLinkProgram(pickingShaderProgram);
        if (!checkCompileErrors(pickingShaderProgram, "PICKING_PROGRAM")) return false;

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

    glm::vec3 readPixelColor(double xpos, double ypos)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
        unsigned char data[3]; // RGB
        glReadPixels(static_cast<int>(xpos), height - 1 - static_cast<int>(ypos), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return glm::vec3(data[0], data[1], data[2]);
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
    float cameraSpeed = 0.05f; // Default camera movement speed
    int selectedSubmeshIndex = -1; // -1 means no submesh is selected

    // Picking FBO and related textures/renderbuffers
    GLuint pickingFBO = 0;
    GLuint pickingTexture = 0;
    GLuint pickingDepthStencilRBO = 0;
    GLuint pickingShaderProgram = 0;

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
        uniform int isSelected;
        void main() {
            if (isSelected == 1) {
                FragColor = vec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow for selected
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
