#include "3DViewer.h"
#include "state.h"
#include <iostream>
#include "portable-file-dialogs.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader.h"

using namespace std;

C3DViewer::C3DViewer() : m_lastMouseX(0.0), m_lastMouseY(0.0)
{
}

C3DViewer::~C3DViewer()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    delete m_state;

    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool C3DViewer::setup()
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

    // Create the state manager, which will in turn create our objects
    m_state = new State();

    glViewport(0, 0, width, height);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, keyCallbackStatic);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallbackStatic);
    glfwSetCursorPosCallback(m_window, cursorPosCallbackStatic);

    return true;
}

void C3DViewer::update()
{
    if (m_state)
    {
        m_state->update();
    }
}

void C3DViewer::mainLoop() 
{
    while (!glfwWindowShouldClose(m_window)) 
    {
        glfwPollEvents();
        update();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        render();

        glfwSwapBuffers(m_window);
    }
}

void C3DViewer::onKey(int key, int scancode, int action, int mods) 
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
}

void C3DViewer::onMouseButton(int button, int action, int mods) 
{
    std::cout << "onMouseButton called. Button: " << button << ", Action: " << action << std::endl;
    ImGui_ImplGlfw_MouseButtonCallback(m_window, button, action, mods);

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            m_rightMouseDown = true;
            glfwGetCursorPos(m_window, &m_lastMouseX, &m_lastMouseY); // Get current mouse pos when button pressed
        }
        else if (action == GLFW_RELEASE)
        {
            m_rightMouseDown = false;
        }
    }
}

void C3DViewer::onCursorPos(double xpos, double ypos) 
{
    ImGui_ImplGlfw_CursorPosCallback(m_window, xpos, ypos);

    if (m_rightMouseDown)
    {
        double deltaX = xpos - m_lastMouseX;
        double deltaY = ypos - m_lastMouseY;
        handleMouseRotation(deltaX, deltaY);
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
    }
}

void C3DViewer::handleMouseRotation(double deltaX, double deltaY)
{
    if (!m_state) return;

    float sensitivity = 0.2f; 
    float rotationAmountY = deltaX * sensitivity; // Y-axis rotation (yaw)
    float rotationAmountX = deltaY * sensitivity; // X-axis rotation (pitch)

    // Call a new State method to rotate the object
    m_state->rotateObject(rotationAmountX, rotationAmountY);
}

void C3DViewer::render() 
{
    glUseProgram(m_shaderProgram);

    // Create and set view and projection matrices
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    GLint viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
    GLint projectionLoc = glGetUniformLocation(m_shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Tell the state manager to draw all its objects
    if (m_state)
    {
        m_state->draw(m_shaderProgram);
    }

    drawInterface();
}

void C3DViewer::drawInterface()
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
            std::cout << "Selected file: " << selection[0] << std::endl;
            if (m_state) {
                try {
                    m_state->load_object(selection[0]);
                } catch (const std::exception& e) {
                    std::cerr << "Error loading object: " << e.what() << std::endl;
                }
            }
        }
    }

    static int anyDummyValue = 50;
    if (ImGui::SliderInt("slider-demo", &anyDummyValue, 1, 100)) 
    {
        // ...
    }
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void C3DViewer::resize(int new_width, int new_height) 
{
    width = new_width;
    height = new_height;
    glViewport(0, 0, width, height);
}

bool C3DViewer::setupShader() 
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

bool C3DViewer::checkCompileErrors(GLuint shader, const char* type) 
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

// Static callbacks
void C3DViewer::keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods) 
{
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    C3DViewer* self = (C3DViewer*)glfwGetWindowUserPointer(window);
    if (self) self->onKey(key, scancode, action, mods);
}

void C3DViewer::mouseButtonCallbackStatic(GLFWwindow* window, int button, int action, int mods) 
{
    std::cout << "mouseButtonCallbackStatic called. Button: " << button << ", Action: " << action << std::endl;
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    C3DViewer* self = (C3DViewer*)glfwGetWindowUserPointer(window);
    if (self) self->onMouseButton(button, action, mods);
}

void C3DViewer::cursorPosCallbackStatic(GLFWwindow* window, double xpos, double ypos) 
{
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    C3DViewer* self = (C3DViewer*)glfwGetWindowUserPointer(window);
    if (self) self->onCursorPos(xpos, ypos);
}
