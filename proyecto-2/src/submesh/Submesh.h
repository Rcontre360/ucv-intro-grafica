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
#include "../utils/Camera.h" // Added for Camera::getInstance()

using namespace std;

const float NORMAL_DIAGONAL_PERCENTAGE = 0.05f; // New constant

class Submesh : public BaseSubmesh 
{
public:
    bool showBoundingBox = false;
    float boundingBoxColor[3] = { 1.0f, 0.0f, 1.0f };
    BaseSubmesh* boundingBox = nullptr;
    BaseSubmesh* normals = nullptr;

    Submesh(const vector<Vertex>& vertices) : BaseSubmesh(vertices){
        BoundingBox box = setupBoundingBox(vertices);
        setupNormals(vertices,box);
    }

    ~Submesh()
    {
        delete boundingBox;
        delete normals;
    }

    void draw(const DrawConfig& config){
        BaseSubmesh::draw(config);
        
        if (showBoundingBox && boundingBox) {
            boundingBox->translate = getTransform();
            boundingBox->drawAsLines(config.shaderProgram, boundingBoxColor);
        }

        if (config.showNormals && normals) {
            glm::mat4 model = getTransform();
            // Use view and projection from config, already calculated in C3DViewer::render
            // glm::mat4 view = Camera::getInstance().getViewMatrix(); // REMOVED
            // glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)config.width / (float)config.height, 0.1f, 100.0f); // REMOVED

            // Calculate normal length based on this submesh's bounding box diagonal
            BoundingBox localBounds = makeBoundingBox(this->vertices);
            float diagonalLength = glm::distance(localBounds.min, localBounds.max);
            float currentNormalLength = diagonalLength * NORMAL_DIAGONAL_PERCENTAGE;

            normals->drawCorrectlyTransformedLines(config, model, config.normalColor, currentNormalLength);
            glUseProgram(config.shaderProgram); // Restore the main shader program
        }
    }

private:
    BoundingBox setupBoundingBox(const vector<Vertex>& vertices) {
        BoundingBox box = makeBoundingBox(vertices);
        auto [minBound,maxBound,_] = box;

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
        
        vector<Vertex> bbox_vertices;
        for(int i = 0; i < 24; ++i) {
            Vertex vertex;
            vertex.position = {v[indices[i]*3+0], v[indices[i]*3+1], v[indices[i]*3+2]};
            vertex.color = {1.0,1.0,1.0};
            bbox_vertices.push_back(vertex);
        }

        boundingBox = new BaseSubmesh(bbox_vertices);
        return box;
    }

    void setupNormals(const vector<Vertex>& vertices, BoundingBox box) {
        vector<Vertex> normal_vertices;
        for (const auto& vertex : vertices) {
            Vertex v;
            v.position = vertex.position;
            v.normal = vertex.normal; // Store original normal
            v.color = {1.0, 1.0, 0.0}; // Yellow for normals
            normal_vertices.push_back(v);
        }
        normals = new BaseSubmesh(normal_vertices);
    }
};
