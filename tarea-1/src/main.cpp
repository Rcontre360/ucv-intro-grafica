#include "PixelRender.h"
#include <iostream>
#include <cmath>
#include <random>
// std::pair
#include <utility>
#include <algorithm>

using namespace std;

// distribution to generate random numbers
typedef uniform_int_distribution<int> Dist;

//line made of 2 points, color and thickness
struct Line {
    pair<pair<int,int>, pair<int,int>> coord;
    RGBA color;
    int thickness;
};

class CMyTest : public CPixelRender
{
protected:
    //current color config
    RGBA color = {255, 255, 255, 255};

    //wether or not to use bresenham
    bool use_bresenham = true;

    //frames by second cache
    int frames_by_second = 0;

    //special value for thickness
    int thickness = 0;

    //coordenades for each line
    int m_x0 = -1, m_y0 = -1, m_x1 = -1, m_y1 = -1;

    //amount or random lines
    int RAND_LINES = 1000;

    //lines created
    vector<Line> lines;

    // rand num generation
    mt19937 rand_gen;

    // draw line with Bresenham algorithm
    // the 4 cases are contemplated by comparing dx < dy (we are moving faster on x or on y, so we are going to use one or the other)
    // if we move faster on x, we iterate on x, otherwise we iterate on y and all calculations using dx and dy are swapped
    // the other 2 cases are when dx < 0 or dy < 0 (we are drawing towards the left or down). Here we just multiply those by -1
    void drawLineWithBresenham(pair<int,int> a, pair<int,int> b, RGBA color){
        int dx = b.first - a.first;
        int dy = b.second - a.second;
        int d = dx - 2*dy;
        int x_inc = dx < 0 ? -1 : 1;
        int y_inc = dy < 0 ? -1 : 1;

        if (dx < 0)
            dx *= -1;
        if (dy < 0)
            dy *= -1;

        bool run_on_x = dx >= dy;
        int inc_e = -2*(run_on_x ? dy : dx);
        int inc_ne = 2*(dx-dy) * (run_on_x ? 1 : -1);

        int x = a.first;
        int y = a.second;
        setPixel(x,y,color);

        // we are drawing within the range of the second point
        while (x > b.first || x < b.first){
            if (d <= 0){
                d+=inc_ne;
                run_on_x ? y+=y_inc : x+=x_inc;
            } else 
                d+=inc_e;

            // we increase x or y depending on the case (dx >= dy)
            run_on_x ? x+=x_inc : y+=y_inc;
            setPixel(x,y,color);
        }
    }

    // draw line with real numbers
    void drawLineWithReal(pair<int,int> a, pair<int,int> b, RGBA color){
        int den = (b.first - a.first);
        // calculate slope
        float m = den==0? 0 : (float)(b.second - a.second) / (float)den;
        // calculate B constant
        float B = (float)a.second - m*(float)a.first;

        // dx and dy must always be positive. This is used to iterate over x or y (dx>=dy)
        int dx = abs(b.first - a.first), dy = abs(b.second - a.second);
        // the start or end depends on which we are using to iterate (x or y). 
        // start is the lowest of the start point (x or y). End is the max of those
        // we could skip this max/min if we allow to iterate downards (i--), but its cleaner to always have i++
        int start = dx > dy ? min(a.first,b.first) : min(a.second,b.second);
        int end = dx > dy ? max(a.first,b.first) : max(a.second,b.second);

        // we iterate over start towards end
        for (int i = start; i < end; ++i)
        {
            // we are iterating over x or over y? dx > dy is x since we move faster on x
            if (dx > dy){
                // equation for y = f(x)
                int y = (int)round(m*(float)i+B);
                setPixel(i, y, color);
            } else {
                // equation for x = f(y)
                int x = (int)round(((float)i-B) / m);
                setPixel(x, i, color);
            }
        }
    }

    //generates a random color
    RGBA generateRandomColor(){
        Dist c_dist(0, 255);
        return { 
            (unsigned char)(static_cast<unsigned char>(c_dist(rand_gen))), 
            (unsigned char)(static_cast<unsigned char>(c_dist(rand_gen))), 
            (unsigned char)(static_cast<unsigned char>(c_dist(rand_gen))), 
            255 
        };
    }

    //generates a random point (pair)
    pair<int,int> generateRandomPoint(Dist x_dist, Dist y_dist){
        return make_pair(x_dist(rand_gen), y_dist(rand_gen));
    }

    //generates a random line
    Line generateRandomLine(Dist x_dist, Dist y_dist){
        return {
            { 
                generateRandomPoint(x_dist, y_dist), 
                generateRandomPoint(x_dist, y_dist) 
            },
            generateRandomColor()    
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

        // a bit bigger, didnt like the small one
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_Once); // Adjusted size for new controls
        ImGui::Begin("Control Panel");
        // bigger font, im blind xd
        ImGui::SetWindowFontScale(1.5f);

        // I want to keep the RGBA struct above, so we need to use this variable to capture the UI value 
        // and then update above's variable
        int temp_color[3] = {(int)color.r,(int)color.g,(int)color.b};

        //RGB and THICKNESS sliders. Thickness is a new variable we create for something extra ;)
        ImGui::SliderInt("R", &temp_color[0], 0, 255);
        ImGui::SliderInt("G", &temp_color[1], 0, 255);
        ImGui::SliderInt("B", &temp_color[2], 0, 255);
        ImGui::SliderInt("Thickness", &thickness, 0, 10);

        // updating color
        color = { (unsigned char)(temp_color[0]), (unsigned char)(temp_color[1]), (unsigned char)(temp_color[2]), 255 };

        // checkbox for bresenham
        ImGui::Checkbox("Use Bresenham", &use_bresenham);

        // button to generate random lines
        if (ImGui::Button("Generate random", ImVec2(200,35)))
            drawRandomLines(RAND_LINES);
        // button to clear the UI
        if (ImGui::Button("Clear", ImVec2(200,35)))
            lines.clear();

        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // calculate frames for each second. Delta time will reach 1 second after many executions of this function
        // When delta time reaches 1 second, we check how many times we have rendered this function (1 frame for each render)
        // We divide how many frames passed by delta time (which might be above 1), this give us how many frames have appeared in 1 second (aprox)
        if (deltaTime >= 1.0) {
            char title[256];
            snprintf(title, sizeof(title), "frames per second: %.2lf", frames_by_second / deltaTime);
            glfwSetWindowTitle(m_window, title);
            lastTime = currentTime;
            frames_by_second = 0;
        }
    }

    // we create 1000 random lines
    void drawRandomLines(int amount){
        Dist x_dist = Dist(0, width), y_dist = Dist(0,height);
        for (int x = 0; x < amount; x++)
            lines.push_back(generateRandomLine(x_dist,y_dist));
    }

    // NOTE: this function simulates (without much efficiency) line's thickness
    // we draw one line if thickness = 0; We decide which algorithm to use given the UI boolean.
    // if thickness is above 0 we simulate it easily by drawing 1 + 4 * thickness lines. 
    // We draw the thickness lines around the initial points. This way it seems they're part of the same line
    // You will notice that the 2 points where the lines begin/end have "crosses" (+). Thats because we draw
    // all lines above, below and on each side of the original point, to remove this effect we would also need to draw 
    // on the up/left, down/left, etc, etc sides.
    void drawLine(Line line){
        auto a = line.coord.first;
        auto b = line.coord.second;
        auto color = line.color;

        if (use_bresenham)
            drawLineWithBresenham(a,b,color);
        else
            drawLineWithReal(a,b,color);

        // only if thickness is above 0. This is EXTRA and only executed when thickness > 0
        for (int i=0; i < line.thickness; i++){
            if (use_bresenham){
                drawLineWithBresenham(make_pair(a.first - i,a.second),make_pair(b.first - i, b.second),color);
                drawLineWithBresenham(make_pair(a.first,a.second - i),make_pair(b.first,b.second - i),color);
                drawLineWithBresenham(make_pair(a.first + i,a.second),make_pair(b.first + i, b.second),color);
                drawLineWithBresenham(make_pair(a.first,a.second + i),make_pair(b.first,b.second + i),color);
            }else{
                drawLineWithReal(make_pair(a.first - i,a.second),make_pair(b.first - i, b.second),color);
                drawLineWithReal(make_pair(a.first,a.second - i),make_pair(b.first,b.second - i),color);
                drawLineWithReal(make_pair(a.first + i,a.second),make_pair(b.first + i, b.second),color);
                drawLineWithReal(make_pair(a.first,a.second + i),make_pair(b.first,b.second + i),color);
            }
        }
    }

    void update()
    {
        //This condition gives a performance improvement to only render when the UI is updated
        //its commented out because we cannot appreciate the performance difference of both algorithms
        //if (mouseButtonsDown[0]){
        fill(m_buffer.begin(), m_buffer.end(), RGBA{ 0,0,0,0 });

        // we draw all lines
        for (auto x:lines){
            drawLine(x);
        }

        // we only draw the NEW LINE if the ending point is being initialized (after the first click)
        if (m_x1 > -1 && m_y1 > -1){
            Line line = {
                { {m_x0,m_y0}, {m_x1,m_y1} },
                color,
                thickness
            };
            drawLine(line);
        }
        //}
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
                // we click so we initialize the first point to draw the line
                m_x0 = xpos;
                m_y0 = ypos;
                cout << "Mouse button " << button << " pressed at position (" << m_x0 << ", " << m_y0 << ")\n";
            }
            else if (action == GLFW_RELEASE)
            {

                // we release so we store the created line. We reset the coords of the last line
                mouseButtonsDown[button] = false;
                Line cur_line = {
                    { {m_x0,m_y0}, {xpos,ypos} },
                    color,
                    thickness
                };
                lines.push_back(cur_line);
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
            // we update the coordenades of the last point when the mouse moves
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


