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
    GLuint normalShaderProgram; // New: for normal visualization
    glm::mat4 view; // New: View matrix
    glm::mat4 projection; // New: Projection matrix
    int width; // New: for projection aspect ratio
    int height; // New: for projection aspect ratio
    bool showVertices;
    float* vertexColor;
    float normalWidth; // New: for projection aspect ratio
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
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::quat rotate = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::mat4 translate = glm::mat4(1.0f);
    glm::mat4 initialTransform;

    vector<Vertex> vertices;
    float color[3];
    float initialColor[3];

    GLuint vao = 0;
    GLuint vbo = 0;

    int vertexCount = 0;

    BaseSubmesh(const vector<Vertex>& vertices) 
        : vertices(vertices), vertexCount(vertices.size())
    {
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

        // Position attribute (location 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal attribute (location 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Color attribute (location 2)
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

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

    void drawCorrectlyTransformedLines(const DrawConfig& config, const glm::mat4& model, const float* normalColor, float normalLength) {
        glUseProgram(config.normalShaderProgram);

        setGpuVariable(config.normalShaderProgram, Shaders::NormalShader::model, model);
        setGpuVariable(config.normalShaderProgram, Shaders::NormalShader::view, config.view);
        setGpuVariable(config.normalShaderProgram, Shaders::NormalShader::projection, config.projection);
        setGpuVariable(config.normalShaderProgram, Shaders::NormalShader::normalLength, normalLength);
        setGpuVariable(config.normalShaderProgram, Shaders::NormalShader::u_normal_color, glm::make_vec3(normalColor));
        
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0, -1.0);

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, vertexCount); // Changed to GL_POINTS for Geometry Shader input
        glBindVertexArray(0);

        glDisable(GL_POLYGON_OFFSET_LINE);
    }

    vector<Vertex> getWorldVertices() const {
        glm::mat4 transformMat = getTransform();

        vector<Vertex> worldV;
        worldV.reserve(vertices.size());

        for (const auto& v : vertices) {
            Vertex res = v;
            glm::vec4 worldPos = transformMat * glm::vec4(v.position, 1.0f);
            res.position = glm::vec3(worldPos);

            worldV.push_back(res);
        }

        return worldV;
    }

    void initTranslate(const glm::mat4& newTransform) { 
        translate = newTransform; 
    }

    void setTranslate(const glm::vec3& offset) { 
        translate = glm::translate(glm::mat4(1.0f), offset) * translate;
    }

    void setRotate(float angle, const glm::vec3& axis) { 
        glm::quat q = glm::angleAxis(glm::radians(angle), axis);
        rotate = q * rotate;
    }

    void setRotate(float angle, const glm::vec3& axis, const glm::vec3& center) { 
        glm::quat rotationQuad = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
        rotate = rotationQuad * rotate;

        glm::vec3 pos = glm::vec3(translate[3]);
        glm::vec3 dirToPivot = pos - center;

        glm::vec3 newDir = rotationQuad * dirToPivot;
        glm::vec3 delta = (center + newDir) - pos;

        setTranslate(delta);
    }

    void setScale(const glm::vec3& factor) { 
        scale *= factor;
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
        glm::mat4 R = glm::mat4(rotate);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        return translate * R * S;
    }

    void resetTransform() { 
        translate = glm::mat4(1.0f); 
        rotate = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); 
        scale = glm::vec3(1.0f); 
    }

private:
    void drawFill(const DrawConfig& config) {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void drawWireframe(const DrawConfig& config) {
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, 1);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_color, glm::make_vec3(config.wireframeColor));
        glEnable(GL_POLYGON_OFFSET_LINE); // Enable polygon offset for lines
        glPolygonOffset(-1.0f, -1.0f); // Offset parameters
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_POLYGON_OFFSET_LINE); // Disable polygon offset
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, 0);
    }

    void drawVertices(const DrawConfig& config) {
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, 1);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_color, glm::make_vec3(config.vertexColor));
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_point_size, config.pointSize);
        glEnable(GL_POLYGON_OFFSET_POINT); // Enable polygon offset for points
        glPolygonOffset(-1.0f, -1.0f); // Offset parameters
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, vertexCount);
        glDisable(GL_POLYGON_OFFSET_POINT); // Disable polygon offset
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, 0);
    }
};
