#include "PixelRender.h"
#include <iostream>
#include <cmath>
// std::pair
#include <utility>
#include <algorithm>

using namespace std;

using LINE = pair<pair<int,int>, pair<int,int>>;

class CMyTest : public CPixelRender
{
protected:
    float line_color[3] = {255,255,255};
    bool use_bresenham = true;
    int frames_by_second = 0;
    int m_x0 = -1, m_y0 = -1, m_x1 = -1, m_y1 = -1;

    vector<LINE> lines;

public:
    CMyTest() {};
    ~CMyTest() {};

    void drawInterface()
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;

        frames_by_second+=1;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(350, 200), ImGuiCond_Once); // Adjusted size for new controls
        ImGui::Begin("Control Panel");
        ImGui::SetWindowFontScale(1.5f);

        ImGui::SliderFloat("R", &line_color[0], 0, 255);
        ImGui::SliderFloat("G", &line_color[1], 0, 255);
        ImGui::SliderFloat("B", &line_color[2], 0, 255);

        if (ImGui::Checkbox("Use Bresenham", &use_bresenham)) {
            printf("Wireframe toggled: %s\n", use_bresenham ? "true" : "false");
        }

        if (ImGui::Button("Generate random", ImVec2(200,35))) {
            printf("Button 'Recalculate Data' pressed!\n");
        }
        
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (deltaTime >= 1.0) {
            char title[256];
            snprintf(title, sizeof(title), "frames per second: %.2lf", frames_by_second / deltaTime);
            glfwSetWindowTitle(m_window, title);
            lastTime = currentTime;
            frames_by_second = 0;
        }
    }

    void update()
    {
        std::fill(m_buffer.begin(), m_buffer.end(), RGBA{ 0,0,0,0 });
        if ((mouseButtonsDown[0] || mouseButtonsDown[1] || mouseButtonsDown[2]) && m_x1 > -1 && m_y1 > -1)
        {
            int den = (m_x1 - m_x0);
            float m = den==0? 0 : (float)(m_y1 - m_y0) / (float)den;
            float b = (float)m_y0 - m*(float)m_x0;

            for (int i = std::min(m_x0,m_x1); i < std::max(m_x0,m_x1); ++i)
            {
                int y = (int)std::round(m*(float)i+b);
                RGBA color = { (unsigned char)(line_color[0]), (unsigned char)(line_color[1]), (unsigned char)(line_color[2]), 255 };
                setPixel(i, y, color);
            }
        }
    }

    void onKey(int key, int scancode, int action, int mods) 
    {
        if (action == GLFW_PRESS)
        {
            std::cout << "Key " << key << " pressed\n";
            if (key == GLFW_KEY_ESCAPE)
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        }
        else if (action == GLFW_RELEASE)
            std::cout << "Key " << key << " released\n";
    }


    void onMouseButton(int button, int action, int mods) 
    {
        if (button >= 0 && button < 3) 
        {
            double xpos, ypos;
            glfwGetCursorPos(m_window, &xpos, &ypos);
            if (action == GLFW_PRESS)
            {
                mouseButtonsDown[button] = true;
                m_x0 = xpos;
                m_y0 = ypos;
                std::cout << "Mouse button " << button << " pressed at position (" << xpos << ", " << ypos << ")\n";
            }
            else if (action == GLFW_RELEASE)
            {

                mouseButtonsDown[button] = false;
                LINE cur_line = { {m_x0,m_y0}, {m_x1,m_y1} };
                m_x1, m_y1 = -1;
                lines.push_back(cur_line);
                std::cout << "Mouse button " << button << " released at position (" << xpos << ", " << ypos << ")\n";
            }
        }
    }

    void onCursorPos(double xpos, double ypos) 
    {
        if (mouseButtonsDown[0] || mouseButtonsDown[1] || mouseButtonsDown[2]) 
        {
            m_x1 = xpos;
            m_y1 = ypos;
        }
    }
};

int main() {
    CMyTest test;
    if (!test.setup()) {
        fprintf(stderr, "Failed to setup CPixelRender\n");
        return -1;
    }

    test.mainLoop();

    return 0;
}
