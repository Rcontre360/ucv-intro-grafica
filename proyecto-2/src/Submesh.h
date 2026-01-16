#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <algorithm>
#include <limits>
#include "GLUtils.h"
#include "Vertex.h"

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
};

class Submesh 
{
public:
    // Transformation matrix
    glm::mat4 transform;

    // OpenGL buffer IDs
    GLuint vao = 0;
    GLuint vbo = 0;
    int vertexCount = 0;
    GLuint textureId = 0;
    bool hasTexture = false;
    float overrideColor[3] = { 1.0f, 1.0f, 1.0f };
    glm::vec3 minBound;
    glm::vec3 maxBound;
    bool showBoundingBox = false;
    float boundingBoxColor[3] = { 1.0f, 0.0f, 1.0f };
    GLuint bboxVao = 0, bboxVbo = 0;

    // Constructor: Initializes transformation and creates OpenGL buffers for the object's geometry.
    Submesh(const std::vector<Vertex>& vertices, GLuint textureId = 0) 
        : transform(glm::mat4(1.0f)), vertexCount(vertices.size()), textureId(textureId)
    {
        hasTexture = (textureId != 0);
        minBound = glm::vec3(std::numeric_limits<float>::max());
        maxBound = glm::vec3(std::numeric_limits<float>::lowest());
        for (const auto& vertex : vertices) {
            minBound.x = std::min(minBound.x, vertex.position.x);
            minBound.y = std::min(minBound.y, vertex.position.y);
            minBound.z = std::min(minBound.z, vertex.position.z);
            maxBound.x = std::max(maxBound.x, vertex.position.x);
            maxBound.y = std::max(maxBound.y, vertex.position.y);
            maxBound.z = std::max(maxBound.z, vertex.position.z);
        }

        std::vector<float> flatVertices = Vertex::flatten(vertices);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, flatVertices.size() * sizeof(float), flatVertices.data(), GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Color attribute
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // Texture coordinate attribute
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);

        setupBoundingBox();
    }

    // Destructor: Cleans up OpenGL buffers.
    ~Submesh()
    {
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        if (bboxVbo) glDeleteBuffers(1, &bboxVbo);
        if (bboxVao) glDeleteVertexArrays(1, &bboxVao);
        if (hasTexture && textureId) glDeleteTextures(1, &textureId);
    }

    void drawForPicking(GLuint shaderProgram)
    {
        setGpuVariable(shaderProgram, "model", transform);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    // Draws the submesh using the provided shader program.
    void draw(const DrawConfig& config)
    {
        setGpuVariable(config.shaderProgram, "model", transform);
        
        if (config.showFill) {
            drawFill(config);
        }

        if (config.showWireframe && config.wireframeColor) {
            drawWireframe(config);
        }

        if (showBoundingBox) {
            drawBoundingBox(config);
        }

        if (config.showVertices && config.vertexColor) {
            drawVertices(config);
        }
    }

    void drawNormals()
    {
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, vertexCount);
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

private:
    void drawFill(const DrawConfig& config) {
        setGpuVariable(config.shaderProgram, "uHasTexture", hasTexture);
        if (hasTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureId);
            setGpuVariable(config.shaderProgram, "uTexture", 0);
        }
        setGpuVariable(config.shaderProgram, "u_render_points", 0);
        setGpuVariable(config.shaderProgram, "u_is_wireframe", 0);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void drawWireframe(const DrawConfig& config) {
        setGpuVariable(config.shaderProgram, "u_is_wireframe", 1);
        setGpuVariable(config.shaderProgram, "u_wireframe_color", glm::make_vec3(config.wireframeColor));
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        setGpuVariable(config.shaderProgram, "u_is_wireframe", 0);
    }

    void drawBoundingBox(const DrawConfig& config) {
        setGpuVariable(config.shaderProgram, "u_is_wireframe", 1);
        setGpuVariable(config.shaderProgram, "u_wireframe_color", glm::make_vec3(boundingBoxColor));
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0, -1.0);
        glBindVertexArray(bboxVao);
        glDrawArrays(GL_LINES, 0, 24);
        glBindVertexArray(0);
        glDisable(GL_POLYGON_OFFSET_LINE);
        setGpuVariable(config.shaderProgram, "u_is_wireframe", 0);
    }

    void drawVertices(const DrawConfig& config) {
        setGpuVariable(config.shaderProgram, "u_render_points", 1);
        setGpuVariable(config.shaderProgram, "vertexColor", glm::make_vec3(config.vertexColor));
        setGpuVariable(config.shaderProgram, "u_point_size", config.pointSize);
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, vertexCount);
    }

    void setupBoundingBox() {
        float vertices[] = {
            minBound.x, minBound.y, minBound.z,
            maxBound.x, minBound.y, minBound.z,
            maxBound.x, maxBound.y, minBound.z,
            minBound.x, maxBound.y, minBound.z,
            minBound.x, minBound.y, maxBound.z,
            maxBound.x, minBound.y, maxBound.z,
            maxBound.x, maxBound.y, maxBound.z,
            minBound.x, maxBound.y, maxBound.z
        };

        unsigned int indices[] = {
            0, 1, 1, 2, 2, 3, 3, 0,
            4, 5, 5, 6, 6, 7, 7, 4,
            0, 4, 1, 5, 2, 6, 3, 7
        };
        
        float line_vertices[24 * 3];
        for(int i = 0; i < 24; ++i) {
            line_vertices[i*3+0] = vertices[indices[i]*3+0];
            line_vertices[i*3+1] = vertices[indices[i]*3+1];
            line_vertices[i*3+2] = vertices[indices[i]*3+2];
        }

        glGenVertexArrays(1, &bboxVao);
        glGenBuffers(1, &bboxVbo);

        glBindVertexArray(bboxVao);
        glBindBuffer(GL_ARRAY_BUFFER, bboxVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};
