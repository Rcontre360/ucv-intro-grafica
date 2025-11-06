#include "PixelRender.h"
#include <iostream>
#include <cmath>
// std::pair
#include <utility>
#include <algorithm>

using namespace std;

struct LINE {
    pair<pair<int,int>, pair<int,int>> coord;
    RGBA color;
};

class CMyTest : public CPixelRender
{
protected:
    float line_color[3] = {255,255,255};
    bool use_bresenham = true;
    int frames_by_second = 0;
    int m_x0 = -1, m_y0 = -1, m_x1 = -1, m_y1 = -1;

    vector<LINE> lines;


    RGBA get_color(){
        return { (unsigned char)(line_color[0]), (unsigned char)(line_color[1]), (unsigned char)(line_color[2]), 255 };
    }

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

    void drawLineWithBresenham(pair<int,int> a, pair<int,int> b, RGBA color){
        int dx,dy,x,y,d,inc_e,inc_ne, x_inc;
        dx = b.first - a.first;
        dy = b.second - a.second;
        d = dx - 2*dy;

        x = a.first;
        y = a.second;
        setPixel(x,y,color);

        if (dx < 0){
            dx *= -1;
            x_inc=-1;
        } else {
            x_inc=1;
        }

        int by_x = dx >= dy;

        inc_e = -2*(by_x ? dy : dx);
        inc_ne = 2*(dx-dy) * (by_x ? 1 : -1);

        while (y < b.second){
            if (d <= 0){
                d+=inc_ne;
                by_x ? y++ : x+=x_inc;
            } else 
                d+=inc_e;

            by_x ? x+=x_inc : y++;
            setPixel(x,y,color);
        }
    }

    void drawLineWithReal(pair<int,int> a, pair<int,int> b, RGBA color){
        int den = (b.first - a.first);
        float m = den==0? 0 : (float)(b.second - a.second) / (float)den;
        float B = (float)a.second - m*(float)a.first;

        for (int i = std::min(a.first,b.first); i < std::max(a.first,b.first); ++i)
        {
            int y = (int)std::round(m*(float)i+B);
            setPixel(i, y, color);
        }
    }

    void drawLine(pair<int,int> a, pair<int,int> b, RGBA color){
        if (use_bresenham)
            drawLineWithBresenham(a,b,color);
        else
            drawLineWithReal(a,b,color);
    }

    void update()
    {

        if ((mouseButtonsDown[0] || mouseButtonsDown[1] || mouseButtonsDown[2]) && m_x1 > -1 && m_y1 > -1)
        {
            std::fill(m_buffer.begin(), m_buffer.end(), RGBA{ 0,0,0,0 });

            RGBA color = get_color();
            for (auto x:lines){
                auto coord = x.coord;
                RGBA color = x.color;
                drawLine(coord.first,coord.second, color);
            }

            drawLine({m_x0, m_y0}, {m_x1,m_y1}, color);
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
            ypos = height - ypos;
            if (action == GLFW_PRESS)
            {
                mouseButtonsDown[button] = true;
                m_x0 = xpos;
                m_y0 = ypos;
                std::cout << "Mouse button " << button << " pressed at position (" << m_x0 << ", " << m_y0 << ")\n";
            }
            else if (action == GLFW_RELEASE)
            {

                mouseButtonsDown[button] = false;
                LINE cur_line = {
                    { {m_x0,m_y0}, {xpos,ypos} },
                    get_color()    
                };
                lines.push_back(cur_line);
                std::cout << "Mouse button " << button << " released at position (" << m_x1 << ", " << m_y1 << ")\n";
                m_x1 = -1; 
                m_y1 = -1;
            }
        }
    }

    void onCursorPos(double xpos, double ypos) 
    {
        if (mouseButtonsDown[0] || mouseButtonsDown[1] || mouseButtonsDown[2]) 
        {
            m_x1 = xpos;
            m_y1 = height - ypos;
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
