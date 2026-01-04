#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Submesh 
{
public:
    // Transformation matrix
    glm::mat4 transform;

    // OpenGL buffer IDs
    GLuint vao = 0;
    GLuint vbo = 0;
    int vertexCount = 0;

    // Constructor: Initializes transformation and creates OpenGL buffers for the object's geometry.
    Submesh(float* vertices, size_t verticesSize, int count) 
        : transform(glm::mat4(1.0f)), vertexCount(count)
    {
        // Generate a Vertex Array Object (VAO) and a Vertex Buffer Object (VBO)
        // VAO stores the configuration of how to read vertex data from VBOs.
        // VBO stores the actual vertex data (positions, colors, etc.) in GPU memory.
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        // Bind the VAO first, then bind and set vertex buffers, and then configure vertex attributes.
        glBindVertexArray(vao);

        // Bind the VBO and upload the vertex data to it.
        // GL_STATIC_DRAW indicates that the data will not change often.
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, verticesSize, vertices, GL_STATIC_DRAW);

        // Configure the position attribute (layout location 0 in the shader)
        // 0: attribute location in shader
        // 3: number of components per vertex attribute (x, y, z)
        // GL_FLOAT: type of data
        // GL_FALSE: don't normalize
        // 6 * sizeof(float): stride (distance between consecutive vertex attributes - 3 for pos + 3 for color)
        // (void*)0: offset of the first component of the first vertex
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0); // Enable the vertex attribute

        // Configure the color attribute (layout location 1 in the shader)
        // 1: attribute location in shader
        // 3: number of components per vertex attribute (r, g, b)
        // GL_FLOAT: type of data
        // GL_FALSE: don't normalize
        // 6 * sizeof(float): stride (distance between consecutive vertex attributes - 3 for pos + 3 for color)
        // (void*)(3 * sizeof(float)): offset of the first color component (after 3 position floats)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1); // Enable the vertex attribute

        glBindVertexArray(0); // Unbind the VAO to prevent accidental modification
    }

    // Destructor: Cleans up OpenGL buffers.
    ~Submesh()
    {
        // Delete the VBO and VAO from GPU memory
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
    }

    // Draws the submesh using the provided shader program.
    void draw(GLuint shaderProgram)
    {
        // Get the location of the "model" uniform in the shader program
        GLint model = glGetUniformLocation(shaderProgram, "model");
        // Pass the transformation matrix to the shader's "model" uniform
        glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(transform));

        // Bind the VAO that contains the buffer and attribute configurations for this submesh
        glBindVertexArray(vao);
        // Draw the triangles. 
        // GL_TRIANGLES: specifies that the vertices are interpreted as individual triangles.
        // 0: starting index in the enabled arrays.
        // vertexCount: number of vertices to be rendered.
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void translate(const glm::vec3& offset) { 
        transform = glm::translate(transform, offset); 
    }

    void rotate(float angle, const glm::vec3& axis) { 
        transform = glm::rotate(transform, glm::radians(angle), axis); 
    }

    void scale(const glm::vec3& factor) { 
        transform = glm::scale(transform, factor); 
    }

    const glm::mat4& getTransform() const { 
        return transform; 
    }

    void setTransform(const glm::mat4& newTransform) { 
        transform = newTransform; 
    }

    void resetTransform() { 
        transform = glm::mat4(1.0f); 
    }

    void rotateAroundPoint(float angle, const glm::vec3& axis, const glm::vec3& pivot) {
        // Translate to pivot, rotate, then translate back
        transform = glm::translate(glm::mat4(1.0f), pivot) * 
                    glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis) * 
                    glm::translate(glm::mat4(1.0f), -pivot) * 
                    transform;
    }
};
