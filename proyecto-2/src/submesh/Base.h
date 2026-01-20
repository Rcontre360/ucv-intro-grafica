#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <algorithm>
#include <limits>
#include "../utils/Utils.h"

using namespace std;

struct DrawConfig {
    GLuint shaderProgram;
    bool isSelected;
    bool showVertices;
    float* vertexColor;
    float pointSize;
    bool showWireframe;
    float* wireframeColor;
    bool showFill;
    bool showNormals;
    float* normalColor;
};


class BaseSubmesh 
{
public:
    glm::mat4 transform;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint textureId = 0;

    int vertexCount = 0;
    bool hasTexture = false;

    BaseSubmesh(const std::vector<Vertex>& vertices, GLuint textureId) 
        : transform(glm::mat4(1.0f)), vertexCount(vertices.size()), textureId(textureId)
    {
        hasTexture = (textureId != 0);

        std::vector<float> flatVertices = Vertex::flatten(vertices);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, flatVertices.size() * sizeof(float), flatVertices.data(), GL_STATIC_DRAW);

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

    ~BaseSubmesh()
    {
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        if (hasTexture && textureId) glDeleteTextures(1, &textureId);
    }

    void drawForPicking(GLuint shaderProgram)
    {
        setGpuVariable(shaderProgram, "model", transform);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    // Draws the submesh using the provided shader program.
    virtual void draw(const DrawConfig& config)
    {
        setGpuVariable(config.shaderProgram, "model", transform);
        
        if (config.showFill) {
            drawFill(config);
        }

        if (config.showWireframe && config.wireframeColor) {
            drawWireframe(config);
        }

        if (config.showVertices && config.vertexColor) {
            drawVertices(config);
        }
    }

    void drawAsLines(GLuint shaderProgram, float* color) {
        setGpuVariable(shaderProgram, "model", transform);

        setGpuVariable(shaderProgram, "uHasTexture", 0);
        setGpuVariable(shaderProgram, "uHasColor", 1);
        setGpuVariable(shaderProgram, "u_color", glm::make_vec3(color));

        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0, -1.0);
        glBindVertexArray(vao);
        glDrawArrays(GL_LINES, 0, vertexCount);
        glBindVertexArray(0);
        glDisable(GL_POLYGON_OFFSET_LINE);
        setGpuVariable(shaderProgram, "uHasColor", 0);
    }

    void translate(const glm::vec3& offset) { 
        transform = glm::translate(glm::mat4(1.0f), offset) * transform;
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

private:
    void drawFill(const DrawConfig& config) {
        setGpuVariable(config.shaderProgram, "uHasTexture", hasTexture);
        if (hasTexture) {
            setGpuVariable(config.shaderProgram, "uHasColor", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureId);
            setGpuVariable(config.shaderProgram, "uTexture", 0);
        }
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void drawWireframe(const DrawConfig& config) {
        setGpuVariable(config.shaderProgram, "uHasColor", 1);
        setGpuVariable(config.shaderProgram, "u_color", glm::make_vec3(config.wireframeColor));
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        setGpuVariable(config.shaderProgram, "uHasColor", 0);
    }

    void drawVertices(const DrawConfig& config) {
        setGpuVariable(config.shaderProgram, "uHasColor", 1);
        setGpuVariable(config.shaderProgram, "u_color", glm::make_vec3(config.vertexColor));
        setGpuVariable(config.shaderProgram, "u_point_size", config.pointSize);
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, vertexCount);
        setGpuVariable(config.shaderProgram, "uHasColor", 0);
    }
};
