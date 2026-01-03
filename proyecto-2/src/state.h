#pragma once

#include <vector>
#include "object.h"

/*
 * Manages the entire application state, including all scene objects.
 * This class is responsible for creating, updating, and drawing objects.
 * It will also expose methods for the UI to interact with the scene.
 */
class State
{
public:
    std::vector<Object*> m_objects;

    // Constructor: Initializes the state and creates the initial objects.
    State()
    {
        // Define pyramid vertices
        float pyramidVertices[] = {
            // positions          // colors
            // base
            -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
            // face 1
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
             0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
             0.0f,  0.5f,  0.0f,  0.0f, 1.0f, 0.0f,
            // face 2
             0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
             0.0f,  0.5f,  0.0f,  0.0f, 0.0f, 1.0f,
            // face 3
             0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
             0.0f,  0.5f,  0.0f,  1.0f, 1.0f, 0.0f,
            // face 4
            -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
             0.0f,  0.5f,  0.0f,  0.0f, 1.0f, 1.0f
        };
        
        // Create the pyramid object and add it to our list of objects
        Object* pyramid = new Object(pyramidVertices, sizeof(pyramidVertices), 18);
        m_objects.push_back(pyramid);
    }

    // Destructor: Cleans up all managed objects.
    ~State()
    {
        for (Object* obj : m_objects)
        {
            delete obj;
        }
        m_objects.clear();
    }

    // Updates the state of all objects (e.g., for animation).
    void update()
    {
        // Animate the first object in the list (our pyramid)
        if (!m_objects.empty())
        {
            glm::mat4 model = glm::mat4(1.0f);
            float x_offset = sin(glfwGetTime()); // Oscillates between -1 and 1
            model = glm::translate(model, glm::vec3(x_offset, 0.0f, -3.0f));
            m_objects[0]->setTransform(model);
        }
    }

    // Draws all objects managed by the state.
    void draw(GLuint shaderProgram)
    {
        for (Object* obj : m_objects)
        {
            obj->draw(shaderProgram);
        }
    }

    // --- UI-related functions would go here ---
    // Example:
    // void changeColorOfSelectedObject(glm::vec3 newColor) { ... }
};
