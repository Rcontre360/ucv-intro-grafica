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
    bool showBoundingBox = false;
    float boundingBoxColor[3] = { 1.0f, 0.0f, 1.0f };
    BaseSubmesh* boundingBox = nullptr;
    BaseSubmesh* normals = nullptr;

    Submesh(const std::vector<Vertex>& vertices) : BaseSubmesh(vertices){
        setupBoundingBox(vertices);
        setupNormals(vertices);
    }

    ~Submesh()
    {
        delete boundingBox;
        delete normals;
    }

    void draw(const DrawConfig& config){
        BaseSubmesh::draw(config);
        
        if (showBoundingBox && boundingBox) {
            boundingBox->transform = transform;
            boundingBox->drawAsLines(config.shaderProgram, boundingBoxColor);
        }

        if (config.showNormals && normals) {
            normals->transform = transform;
            normals->drawAsLines(config.shaderProgram, config.normalColor);
        }
    }

private:
    void setupBoundingBox(const std::vector<Vertex>& vertices) {
        glm::vec3 minBound = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 maxBound = glm::vec3(std::numeric_limits<float>::lowest());
        for (const auto& vertex : vertices) {
            minBound.x = std::min(minBound.x, vertex.position.x);
            minBound.y = std::min(minBound.y, vertex.position.y);
            minBound.z = std::min(minBound.z, vertex.position.z);
            maxBound.x = std::max(maxBound.x, vertex.position.x);
            maxBound.y = std::max(maxBound.y, vertex.position.y);
            maxBound.z = std::max(maxBound.z, vertex.position.z);
        }

        float v[] = {
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
        
        std::vector<Vertex> bbox_vertices;
        for(int i = 0; i < 24; ++i) {
            Vertex vertex;
            vertex.position = {v[indices[i]*3+0], v[indices[i]*3+1], v[indices[i]*3+2]};
            vertex.color = {1.0,1.0,1.0};
            bbox_vertices.push_back(vertex);
        }

        boundingBox = new BaseSubmesh(bbox_vertices);
    }

    void setupNormals(const std::vector<Vertex>& vertices) {
        std::vector<Vertex> normal_vertices;
        for (const auto& vertex : vertices) {
            Vertex v1, v2;
            v1.position = vertex.position;
            v1.color = {1.0,1.0,0.0};
            v2.position = vertex.position + vertex.normal * NORMAL_LENGTH;
            v2.color = {1.0,1.0,0.0};
            normal_vertices.push_back(v1);
            normal_vertices.push_back(v2);
        }
        normals = new BaseSubmesh(normal_vertices);
    }
};
