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
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, verticesSize, vertices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0); // Unbind
    }

    // Destructor: Cleans up OpenGL buffers.
    ~Submesh()
    {
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
    }

    void draw(GLuint shaderProgram)
    {
        GLint model = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(transform));

        // Bind the VAO and draw the object
        glBindVertexArray(vao);
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
};
