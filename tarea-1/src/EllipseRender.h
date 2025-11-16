#pragma once

#include "PixelRender.h"
#include <iostream>
#include <cmath>
#include <random>
#include <algorithm>

using namespace std;

typedef uniform_int_distribution<int> Dist;
typedef long long ll;

struct Point{
    int x;
    int y;
};

struct Ellipse {
    Point center;
    int a,b;
    RGBA color;
};

class EllipseRender : public CPixelRender
{
protected:
    //current color config
    RGBA color = {255, 255, 255, 255};

    //boolean to start test
    bool similarity_test = false;

    //boolean to use optimized algo
    bool use_optimized = false;

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

    Point generateRandomPoint(){
        Dist x_dist(0, width);
        Dist y_dist(0, height);

        return { 
            x_dist(rand_gen),
            y_dist(rand_gen),
        };
    }

    Ellipse generateRandomEllipse(){
        Point a = generateRandomPoint();
        Point b = generateRandomPoint();

        return {
            {(a.x + b.x)>>1, (a.y + b.y)>>1},
            abs(b.x - a.x)>>1, 
            abs(b.y - a.y)>>1,
            color
        };
    }

    void generateRandomEllipses(int num){
        for (int i=0; i < num; i++)
            ellipses.push_back(generateRandomEllipse());
    }

public:
    EllipseRender() {
        random_device rd;
        rand_gen.seed(rd());
    };
    ~EllipseRender() {};

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

        if (ImGui::Button("Clear", ImVec2(200,35)))
            ellipses.clear();

        ImGui::Checkbox("Use Optimized", &use_optimized);

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

        // could just << 1 but the rule is "no multiplications"
        int x_mid = (x - center.x) + (x - center.x);
        int y_mid = (y - center.y) + (y - center.y);

        setPixel(x,y,c);
        setPixel(x - x_mid,y,c);
        setPixel(x,y - y_mid,c);
        setPixel(x - x_mid,y - y_mid,c);
    }

    void drawEllipse2(Ellipse e){
        RGBA c = e.color;

        ll a = e.a;
        ll b = e.b;

        int x = 0;
        int y = b;

        ll d = 4*b*b - 4*a*a*b + a*a;
        ll m_x = 8*b*b; 
        ll m_y = 8*a*a*y - 4*a*a;

        int auxb1 = 12*b*b - m_x;
        int auxb2 = 8*b*b;
        int auxa1 = 8*a*a;
        int auxab = auxb1 + auxa1 - 4*a*a;

        drawSymetric(e.center, {x,y}, c);
        while (m_x < m_y){

            if (d < 0)
                d += m_x + auxb1;
            else {
                d += m_x - m_y + auxab;
                y--;
                m_y -= auxa1;
            }
            x++;
            
            m_x += auxb2;

            drawSymetric(e.center, {x,y}, c);
        }

        int auxa3 = 8*a*a;

        d = b*b*(4*x*x+4*x+1) + a*a*(4*y*y-8*y+4) - 4*a*a*b*b;

        while (y > 0){

            if (d < 0){
                d += m_x - m_y + auxa3;
                x++;
                m_x += auxb2;
            } else 
                d += -m_y + auxa3;

            y--;
            m_y -= auxa1;
            drawSymetric(e.center, {x,y}, c);
        }
    }

    void drawEllipse1(Ellipse e){
        RGBA c = e.color;

        ll a = e.a;
        ll b = e.b;

        int x = 0;
        int y = b;

        ll d = 4*b*b - 4*a*a*b + a*a;

        drawSymetric(e.center, {x,y}, c);
        while (b*b*2*(x+1) < a*a*(2*y-1)){

            if (d < 0)
                d += 4*(b*b*(2*x+3));
            else {
                d += 4*b*b*(2*x+3) + 4*a*a*(-2*y+2);
                y--;
            }
            x++;
            drawSymetric(e.center, {x,y}, c);
        }

        d = b*b*(4*x*x+4*x+1)+a*a*(4*y*y-8*y+4) - 4*a*a*b*b;
        while (y > 0){

            if (d < 0){
                d += 4*(b*b*(2*x+2)+a*a *(-2*y+3));
                x++;
            } else 
                d += 4*a*a*(-2*y+3);

            y--;
            drawSymetric(e.center, {x,y}, c);
        }
    }

    void drawEllipse(Ellipse e){
        if (use_optimized)
            drawEllipse2(e);
        else
            drawEllipse1(e);
    }

    void update()
    {
        fill(m_buffer.begin(), m_buffer.end(), RGBA{ 0,0,0,0 });

        for (auto x:ellipses){
            drawEllipse(x);
        }

        if (m_x1 > -1 && m_y1 > -1){
            Ellipse e = {
                {(m_x0 + m_x1)>>1, (m_y0 + m_y1)>>1},
                abs(m_x1 - m_x0)>>1, 
                abs(m_y1 - m_y0)>>1,
                color
            };
            drawEllipse(e);
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
                Ellipse e = {
                    {(m_x0 + m_x1)>>1, (m_y0 + m_y1)>>1},
                    abs(m_x1 - m_x0)>>1, 
                    abs(m_y1 - m_y0)>>1,
                    color
                };
                ellipses.push_back(e);
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

    RGBA getPixel(int x, int y){
        if (x < 0 || x >= width || y < 0 || y >= height) return {0,0,0};
        return m_buffer[y * width + x];
    }
};
