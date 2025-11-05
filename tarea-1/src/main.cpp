#include "PixelRender.h"
#include <iostream>

class CMyTest : public CPixelRender
{
public:
    CMyTest() {};
    ~CMyTest() {};

    void update()
    {
        std::fill(m_buffer.begin(), m_buffer.end(), RGBA{ 0,0,0,0 });
        pixelsModifiedThisSecond += m_nPixels;
        // tu código actual para modificar pixels...
        for (int i = 0; i < m_nPixels; ++i)
        {
            int x = rand() % width;
            int y = rand() % height;
            RGBA color = { (unsigned char)(rand() % 256), (unsigned char)(rand() % 256), (unsigned char)(rand() % 256), 255 };
            setPixel(x, y, color);
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
                // Obtener posición actual del cursor
                std::cout << "Mouse button " << button << " pressed at position (" << xpos << ", " << ypos << ")\n";
            }
            else if (action == GLFW_RELEASE)
            {

                mouseButtonsDown[button] = false;
                std::cout << "Mouse button " << button << " released at position (" << xpos << ", " << ypos << ")\n";
            }
        }
    }

    void onCursorPos(double xpos, double ypos) 
    {
        if (mouseButtonsDown[0] || mouseButtonsDown[1] || mouseButtonsDown[2]) 
        {
            std::cout << "Mouse move at (" << xpos << ", " << ypos << ")\n";
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
