#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <algorithm>
#include <limits>
#include "../utils/Utils.h"
#include "../utils/Shaders.h"

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
    glm::mat4 transform = glm::mat4(1.0f);
    glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::mat4 initialTransform;

    glm::vec3 minBound = glm::vec3(numeric_limits<float>::max());
    glm::vec3 maxBound = glm::vec3(numeric_limits<float>::lowest());
    glm::vec3 center = glm::vec3(0.0f);

    vector<Vertex> vertices;
    float color[3];
    float initialColor[3];

    GLuint vao = 0;
    GLuint vbo = 0;

    int vertexCount = 0;

    BaseSubmesh(const vector<Vertex>& vertices) 
        : vertices(vertices), vertexCount(vertices.size())
    {
        glm::vec3 minBound = glm::vec3(numeric_limits<float>::max());
        glm::vec3 maxBound = glm::vec3(numeric_limits<float>::lowest());
        for (const auto& vertex : vertices) {
            minBound.x = min(minBound.x, vertex.position.x);
            minBound.y = min(minBound.y, vertex.position.y);
            minBound.z = min(minBound.z, vertex.position.z);
            maxBound.x = max(maxBound.x, vertex.position.x);
            maxBound.y = max(maxBound.y, vertex.position.y);
            maxBound.z = max(maxBound.z, vertex.position.z);
        }

        center = (minBound + maxBound) / 2.0f;

        if (!vertices.empty()) {
            color[0] = vertices[0].color.r;
            color[1] = vertices[0].color.g;
            color[2] = vertices[0].color.b;
        }

        vector<float> flatVertices = Vertex::flatten(vertices);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, flatVertices.size() * sizeof(float), flatVertices.data(), GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    ~BaseSubmesh()
    {
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
    }

    void drawForPicking(GLuint shaderProgram)
    {
        setGpuVariable(shaderProgram, Shaders::PickingShader::model, getTransform());
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    // Draws the submesh using the provided shader program.
    virtual void draw(const DrawConfig& config)
    {
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::model, getTransform());
        
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
        setGpuVariable(shaderProgram, Shaders::DefaultShader::model, getTransform());
        setGpuVariable(shaderProgram, Shaders::DefaultShader::uHasColor, 1);
        setGpuVariable(shaderProgram, Shaders::DefaultShader::u_color, glm::make_vec3(color));
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0, -1.0);
        glBindVertexArray(vao);
        glDrawArrays(GL_LINES, 0, vertexCount);
        glBindVertexArray(0);
        glDisable(GL_POLYGON_OFFSET_LINE);
        setGpuVariable(shaderProgram, Shaders::DefaultShader::uHasColor, 0);
    }

    void translate(const glm::vec3& offset) { 
        transform = glm::translate(glm::mat4(1.0f), offset) * transform;
    }

    void rotate(float angle, const glm::vec3& axis) { 
        glm::quat q = glm::angleAxis(glm::radians(angle), axis);
        orientation = q * orientation;
    }

    void scale(const glm::vec3& factor) { 
        transform = glm::scale(transform, factor); 
    }

    void updateColor() {
        for (auto& vertex : vertices) {
            vertex.color = {color[0], color[1], color[2]};
        }
        vector<float> flatVertices = Vertex::flatten(vertices);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, flatVertices.size() * sizeof(float), flatVertices.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    const glm::mat4 getTransform() const { 
        return transform * glm::mat4(orientation); 
    }

    void setTransform(const glm::mat4& newTransform) { 
        transform = newTransform; 
    }

    void resetTransform() { 
        transform = glm::mat4(1.0f); 
    }

private:
    void drawFill(const DrawConfig& config) {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void drawWireframe(const DrawConfig& config) {
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, 1);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_color, glm::make_vec3(config.wireframeColor));
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, 0);
    }

    void drawVertices(const DrawConfig& config) {
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, 1);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_color, glm::make_vec3(config.vertexColor));
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_point_size, config.pointSize);
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, vertexCount);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, 0);
    }
};
