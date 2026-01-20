#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <algorithm>
#include <limits>

#include "Base.h"
#include "../utils/Utils.h"

using namespace std;

const float NORMAL_LENGTH = 1.1f;

class Submesh : public BaseSubmesh 
{
public:
    glm::vec3 minBound;
    glm::vec3 maxBound;

    bool showBoundingBox = false;
    float boundingBoxColor[3] = { 1.0f, 0.0f, 1.0f };
    GLuint bboxVao = 0, bboxVbo = 0;
    GLuint normalLinesVao = 0, normalLinesVbo = 0;
    int normalCount = 0;

    Submesh(const std::vector<Vertex>& vertices, GLuint textureId = 0) : BaseSubmesh(vertices,textureId){
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

        setupBoundingBox();
        setupNormals(vertices);
    }

    ~Submesh()
    {
        if (bboxVbo) glDeleteBuffers(1, &bboxVbo);
        if (bboxVao) glDeleteVertexArrays(1, &bboxVao);
        if (normalLinesVbo) glDeleteBuffers(1, &normalLinesVbo);
        if (normalLinesVao) glDeleteVertexArrays(1, &normalLinesVao);
    }

    void draw(const DrawConfig& config){
        BaseSubmesh::draw(config);
        
        if (showBoundingBox) {
            drawBoundingBox(config);
        }

        if (config.showNormals) {
            drawNormals(config);
        }
    }

private:
    void drawBoundingBox(const DrawConfig& config) {
        setGpuVariable(config.shaderProgram, "uHasColor", 1);
        setGpuVariable(config.shaderProgram, "u_color", glm::make_vec3(boundingBoxColor));
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0, -1.0);
        glBindVertexArray(bboxVao);
        glDrawArrays(GL_LINES, 0, 24);
        glBindVertexArray(0);
        glDisable(GL_POLYGON_OFFSET_LINE);
        setGpuVariable(config.shaderProgram, "uHasColor", 0);
    }

    void drawNormals(const DrawConfig& config) {
        setGpuVariable(config.shaderProgram, "uHasColor", 1);
        setGpuVariable(config.shaderProgram, "u_color", glm::make_vec3(config.normalColor));
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0, -1.0);
        glBindVertexArray(normalLinesVao);
        glDrawArrays(GL_LINES, 0, normalCount);
        glBindVertexArray(0);
        glDisable(GL_POLYGON_OFFSET_LINE);
        setGpuVariable(config.shaderProgram, "uHasColor", 0);
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

    void setupNormals(const std::vector<Vertex>& vertices) {
        std::vector<float> lines;
        for (const auto& vertex : vertices) {
            lines.push_back(vertex.position.x);
            lines.push_back(vertex.position.y);
            lines.push_back(vertex.position.z);
            lines.push_back(vertex.position.x + vertex.normal.x * NORMAL_LENGTH);
            lines.push_back(vertex.position.y + vertex.normal.y * NORMAL_LENGTH);
            lines.push_back(vertex.position.z + vertex.normal.z * NORMAL_LENGTH);
        }
        normalCount = lines.size() / 3;

        glGenVertexArrays(1, &normalLinesVao);
        glGenBuffers(1, &normalLinesVbo);

        glBindVertexArray(normalLinesVao);
        glBindBuffer(GL_ARRAY_BUFFER, normalLinesVbo);
        glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};
