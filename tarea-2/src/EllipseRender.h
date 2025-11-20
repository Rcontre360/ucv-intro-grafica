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

        if (ImGui::Button("Generate Random", ImVec2(200,35)))
            generateRandomEllipses(1000);
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

    // if we draw an ellipse and shrink "a" as much as possible (towards 0), the height of the ellipse doesnt change and everything looks ok.
    // BUT if we do the same with "b" weird things happen, draw an ellipse and shrink it on b and see how it stops having the same
    // width and seems to not fully form on a horizontal ellipse. 
    // This issue comes from the original algorithm given in class, that algorithm doesnt address this edge case, only addresses it 
    // when "a" shrinks to 0.
    // I solved this issue by drawing a line from center.x - a towards center.x + a. 
    // If there are pixels on y+1 or y-1 (on the same level of that line), we dont draw.
    // If y > 0 we dont draw either since the ellipse is incomplete and this edge case dont apply.
    // Test it yourself. This function should only be executed when (on the ellipse edges on x axis),
    // the space between the top and bottom are 1 and should be lower (0, thus this line)
    void drawEdgeCase(Ellipse e, int x_drawed){
        int a = e.a, b = e.b;
        int _x = e.center.x - a;
        int end = e.center.x - x_drawed;

        //left line
        while (_x < end){
            setPixel(_x,e.center.y,e.color);
            _x++;
        }

        //right line
        _x = e.center.x + x_drawed;
        end = e.center.x + a;
        while (_x < end){
            setPixel(_x,e.center.y,e.color);
            _x++;
        }
    }

    // draw symetric using the center and the current point we are drawing.
    void drawSymetric(Point cn, Point p, RGBA c){
        setPixel(cn.x + p.x, cn.y + p.y, c);
        setPixel(cn.x - p.x, cn.y + p.y, c);
        setPixel(cn.x - p.x, cn.y - p.y, c);
        setPixel(cn.x + p.x, cn.y - p.y, c);
    }

    // the upgraded algorithm to only use +, - and simple comparisons inside our loops
    void drawEllipse2(Ellipse e){
        RGBA c = e.color;

        ll a = e.a;
        ll b = e.b;

        int x = 0;
        int y = b;
        //vars to sum to mx and my to reduce sums inside loop
        int aux1 =4*b*b;

        ll d = 4*b*b - 4*a*a*b + a*a;
        ll m_x = 12*b*b; 
        ll m_y = 8*a*a*y - 4*a*a + aux1;

        int sum_mx = 8*b*b;
        int sum_my = 8*a*a;
        int const_d1 = 4*b*b + 4*a*a;

        drawSymetric(e.center, {x,y}, c);
        while (m_x < m_y){

            if (d < 0)
                d += m_x;
            else {
                d += m_x - m_y + const_d1;
                y--;
                m_y -= sum_my;
            }
            x++;
            
            m_x += sum_mx;

            drawSymetric(e.center, {x,y}, c);
        }

        //edge case, explained above "drawEdgeCase" and our doc
        if (y <= 0)
            drawEdgeCase(e, x);

        //vars to sum to mx and my to reduce sums inside loop
        int aux2 = 8*a*a + 4*b*b;
        int const_d2 = 8*a*a;

        d = b*b*(4*x*x+4*x+1) + a*a*(4*y*y-8*y+4) - 4*a*a*b*b;

        m_x -= aux2;
        m_y -= aux2;

        while (y > 0){

            if (d < 0){
                d += m_x - m_y + const_d2;
                x++;
                m_x += sum_mx;
            } else 
                d -= m_y;

            y--;
            m_y -= sum_my;
            drawSymetric(e.center, {x,y}, c);
        }
    }

    // exact same algorithm used on the guide. Has an issue when the ellipse is almost flat on the x axis.
    // check drawEdgeCase to see more about the edge case and how I solved it.
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

        if (y <= 0)
            drawEdgeCase(e, x);

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
            // (x0 + x1)/2 and (y0 + y1)/2 is the center
            // a = (x1 - x0) / 2
            // b = (y1 - y0) / 2
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
