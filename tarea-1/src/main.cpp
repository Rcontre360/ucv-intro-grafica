#include "PixelRender.h"
#include <iostream>
#include <cmath>
#include <random>
#include <algorithm>

using namespace std;

typedef uniform_int_distribution<int> Dist;

struct Point{
    int x;
    int y;
};

struct Ellipse {
    Point center;
    int a,b;
    RGBA color;
};

class CMyTest : public CPixelRender
{
protected:
    //current color config
    RGBA color = {255, 255, 255, 255};

    //wether or not to use bresenham
    bool use_bresenham = false;

    //frames by second cache
    int frames_by_second = 0;

    //coordenades for each line
    int m_x0 = -1, m_y0 = -1, m_x1 = -1, m_y1 = -1;

    //ellipses created
    vector<Ellipse> ellipses;

    // rand num generation
    mt19937 rand_gen;

    RGBA generateRandomColor(){
        Dist c_dist(0, 255);
        return { 
            (unsigned char)(static_cast<unsigned char>(c_dist(rand_gen))), 
            (unsigned char)(static_cast<unsigned char>(c_dist(rand_gen))), 
            (unsigned char)(static_cast<unsigned char>(c_dist(rand_gen))), 
            255 
        };
    }

public:
    CMyTest() {
        random_device rd;
        rand_gen.seed(rd());
    };
    ~CMyTest() {};

    void drawInterface()
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;

        frames_by_second+=1;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_Once); // Adjusted size for new controls
        ImGui::Begin("Control Panel");
        ImGui::SetWindowFontScale(1.5f);

        int temp_color[3] = {(int)color.r,(int)color.g,(int)color.b};

        ImGui::SliderInt("R", &temp_color[0], 0, 255);
        ImGui::SliderInt("G", &temp_color[1], 0, 255);
        ImGui::SliderInt("B", &temp_color[2], 0, 255);

        color = { (unsigned char)(temp_color[0]), (unsigned char)(temp_color[1]), (unsigned char)(temp_color[2]), 255 };

        ImGui::Checkbox("Use Optimizd", &use_bresenham);

        if (ImGui::Button("Generate random", ImVec2(200,35)))
            printf("UNIMPLEMENTED");
        if (ImGui::Button("Clear", ImVec2(200,35)))
            ellipses.clear();

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

    void drawSymetric(Point center, Point p, RGBA c){
        int x = p.x + center.x;
        int y = p.y + center.y;

        int x_mid = x - center.x;
        int y_mid = y - center.y;

        setPixel(x,y,c);
        setPixel(x - 2*x_mid,y,c);
        setPixel(x,y - 2*y_mid,c);
        setPixel(x - 2*x_mid,y - 2*y_mid,c);
    }

    void drawEllipse(Ellipse e){
        RGBA c = e.color;

        int a = e.a;
        int b = e.b;

        int x = 0;
        int y = b;

        long long d = 4*b*b - 4*a*a*b + a*a;
        long long m_x = 2*b*b*x;
        long long m_y = 2*a*a*y;

        while (m_x < m_y){
            drawSymetric(e.center, {x,y}, c);

            if (d < 0)
                d += 4*(b*b*(2*x+3));
            else {
                d += 4*b*b*(2*x+3) + 4*a*a*(-2*y+2);
                y--;
            }
            x++;

            m_x = 2*b*b*x;
            m_y = 2*a*a*y;
        }

        d = b*b*(4*x*x+4*x+1)+a*a*(4*y*y-8*y+4) - 4*a*a*b*b;
        while (y > 0){
            drawSymetric(e.center, {x,y}, c);

            if (d < 0){
                d += 4*(b*b*(2*x+2)+a*a *(-2*y+3));
                x++;
            } else 
                d += 4*a*a*(-2*y+3);

            y--;
        }
    }

    void update()
    {
        //Performance improvement to only render when the UI is updated
        if (mouseButtonsDown[0]){
        fill(m_buffer.begin(), m_buffer.end(), RGBA{ 0,0,0,0 });

        //for (auto x:ellipses){
            //drawEllipse(x);
        //}

        if (m_x1 > -1 && m_y1 > -1){
            Ellipse e = {
                // scaled center 2*
                {(m_x0 + m_x1)/2, (m_y0 + m_y1)/2},
                // scaled a
                abs(m_x1 - m_x0)/2, 
                // scaled b
                abs(m_y1 - m_y0)/2,
                color
            };
            drawEllipse(e);
        }
        }
    }

    void onKey(int key, int scancode, int action, int mods) 
    {
        if (action == GLFW_PRESS)
        {
            cout << "Key " << key << " pressed\n";
            if (key == GLFW_KEY_ESCAPE)
                glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        }
        else if (action == GLFW_RELEASE)
            cout << "Key " << key << " released\n";
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
                cout << "Mouse button " << button << " pressed at position (" << m_x0 << ", " << m_y0 << ")\n";
            }
            else if (action == GLFW_RELEASE)
            {

                mouseButtonsDown[button] = false;
                //Ellipse cur_line = {
                    //{m_x0,m_y0}, {xpos,ypos},
                    //color
                //};
                //ellipses.push_back(cur_line);
                cout << "Mouse button " << button << " released at position (" << m_x1 << ", " << m_y1 << ")\n";
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
