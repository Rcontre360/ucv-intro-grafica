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

const float NORMAL_LENGTH_BOX = 0.1;

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
            boundingBox->transform = getTransform();
            boundingBox->drawAsLines(config.shaderProgram, boundingBoxColor);
        }

        if (config.showNormals && normals) {
            normals->transform = getTransform();
            normals->drawAsLines(config.shaderProgram, config.normalColor);
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
        float length = glm::distance(box.max , box.min) * NORMAL_LENGTH_BOX;

        for (const auto& vertex : vertices) {
            Vertex v1, v2;
            v1.position = vertex.position;
            v1.color = {1.0,1.0,0.0};
            v2.position = vertex.position + vertex.normal * length;
            v2.color = {1.0,1.0,0.0};
            normal_vertices.push_back(v1);
            normal_vertices.push_back(v2);
        }
        normals = new BaseSubmesh(normal_vertices);
    }
};
