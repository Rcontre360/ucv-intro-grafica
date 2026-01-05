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
    GLuint texture_id = 0;
    bool has_texture = false;

    // Constructor: Initializes transformation and creates OpenGL buffers for the object's geometry.
    Submesh(float* vertices, size_t verticesSize, int count, GLuint tex_id = 0) 
        : transform(glm::mat4(1.0f)), vertexCount(count), texture_id(tex_id)
    {
        has_texture = (texture_id != 0);
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, verticesSize, vertices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Texture coordinate attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    // Destructor: Cleans up OpenGL buffers.
    ~Submesh()
    {
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        if (has_texture && texture_id) glDeleteTextures(1, &texture_id);
    }

    // Draws the submesh using the provided shader program.
    void draw(GLuint shaderProgram, bool isSelected)
    {
        GLint model = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(transform));

        GLint isSelectedLoc = glGetUniformLocation(shaderProgram, "isSelected");
        glUniform1i(isSelectedLoc, isSelected);

        GLint hasTextureLoc = glGetUniformLocation(shaderProgram, "uHasTexture");
        glUniform1i(hasTextureLoc, has_texture);

        if (has_texture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);
        }

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

    void rotateAroundPoint(float angle, const glm::vec3& axis, const glm::vec3& pivot) {
        transform = glm::translate(glm::mat4(1.0f), pivot) * 
                    glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis) * 
                    glm::translate(glm::mat4(1.0f), -pivot) * 
                    transform;
    }
};
