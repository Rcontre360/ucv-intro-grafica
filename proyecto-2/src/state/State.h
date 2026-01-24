#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <GLFW/glfw3.h>

#include "tinyobjloader.h"
#include "../submesh/Submesh.h"
#include "../utils/FileLoader.h"
#include "../utils/Utils.h"
#include "../utils/Shaders.h"

using namespace std;

class State
{
public:
    bool showVertices = false;
    bool showWireframe = false;
    bool showFill = true;
    bool lineAntialiasing = false;
    bool showNormals = false;
    bool moveFullObjectMode = false;
    bool shouldUpdateCenter = false;

    float vertexColor[3] = { 1.0f, 1.0f, 1.0f };
    float pointSize = 5.0f;
    float wireframeColor[3] = { 1.0f, 1.0f, 1.0f };
    float globalBoundingBoxColor[3] = { 0.0f, 1.0f, 0.0f };
    float normalColor[3] = { 1.0f, 1.0f, 0.0f };

    vector<Submesh*> shapes;

    glm::vec3 center = glm::vec3(1.0f);
    GLuint globalBboxVao = 0, globalBboxVbo = 0;
    BaseSubmesh* globalBoundingBox = nullptr;

    State(){}

    ~State()
    {
        for (Submesh* obj : shapes) {
            delete obj;
        }
        shapes.clear();
        delete globalBoundingBox;
    }

    void updateGlobalBoundingBox()
    {
        delete globalBoundingBox;
        globalBoundingBox = nullptr;

        if (shapes.empty()) return;

        glm::vec3 minBound(numeric_limits<float>::max());
        glm::vec3 maxBound(numeric_limits<float>::lowest());

        for (const auto& shape : shapes) {
            for (const auto& vertex : shape->vertices) {
                glm::vec3 worldPos = shape->transform * glm::vec4(vertex.position, 1.0f);
                minBound.x = min(minBound.x, worldPos.x);
                minBound.y = min(minBound.y, worldPos.y);
                minBound.z = min(minBound.z, worldPos.z);
                maxBound.x = max(maxBound.x, worldPos.x);
                maxBound.y = max(maxBound.y, worldPos.y);
                maxBound.z = max(maxBound.z, worldPos.z);
            }
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

        globalBoundingBox = new BaseSubmesh(bbox_vertices);
    }

    void deleteSubmesh(int index)
    {
        if (index >= 0 && index < shapes.size()) {
            delete shapes[index];
            shapes.erase(shapes.begin() + index);
        }
    }

    void setSelected(int index, bool selected){
        if (index >= 0 && index < shapes.size())
            shapes[index]->showBoundingBox = selected;
    }

    void draw(GLuint shaderProgram, int selectedSubmeshIndex)
    {
        DrawConfig config;
        config.shaderProgram = shaderProgram;
        config.showVertices = showVertices;
        config.vertexColor = vertexColor;
        config.pointSize = pointSize;
        config.showWireframe = showWireframe;
        config.wireframeColor = wireframeColor;
        config.showFill = showFill;
        config.showNormals = showNormals;
        config.normalColor = normalColor;

        for (size_t i = 0; i < shapes.size(); ++i) {
            config.isSelected = ((int)i == selectedSubmeshIndex);
            shapes[i]->draw(config);
        }

        if (moveFullObjectMode && globalBoundingBox) {
            globalBoundingBox->drawAsLines(shaderProgram, globalBoundingBoxColor);
        }
    }

    void drawPicking(GLuint pickingShaderProgram)
    {
        for (size_t i = 0; i < shapes.size(); ++i)
        {
            setGpuVariable(pickingShaderProgram, Shaders::PickingShader::objectId, (int)(i + 1));
            shapes[i]->drawForPicking(pickingShaderProgram);
        }
    }

    void loadObject(const string& path)
    {
        for (Submesh* obj : shapes) {
            delete obj;
        }
        shapes.clear();

        LoadedObject loaded = FileLoader::loadObject(path);
        shapes = loaded.shapes;

        centerShape(loaded);

        for (Submesh* obj : shapes) {
            obj->initialTransform = obj->getTransform();
            obj->initialColor[0] = obj->color[0];
            obj->initialColor[1] = obj->color[1];
            obj->initialColor[2] = obj->color[2];
        }
    }

    void rescaleShape(size_t shape_index, float factor)
    {
        if (shape_index < shapes.size()) {
            shapes[shape_index]->scale(glm::vec3(factor));
        }
    }

    void rescaleAllShapes(float factor)
    {
        for (Submesh* obj : shapes) {
            obj->scale(glm::vec3((float)(factor / oldScale)));
        }
        oldScale = factor;
    }

    void translateFullObject(const glm::vec3& offset)
    {
        if (shapes.empty()) return;

        for (Submesh* obj : shapes) {
            obj->translate(offset); 
        }
        globalBoundingBox->translate(offset);
    }

    void translateSubmesh(int index, const glm::vec3& offset)
    {
        shouldUpdateCenter = true;
        if (index >= 0 && index < shapes.size()) {
            shapes[index]->translate(offset);
        }
    }

    void rotateObject(float angleX, float angleY)
    {
        if (shapes.empty()) return;

        if (shouldUpdateCenter){
            updateCenter();
            shouldUpdateCenter = false;
        }

        for (Submesh* obj : shapes) {
            obj->rotate(angleX, glm::vec3(1.0f, 0.0f, 0.0f), center); 
            obj->rotate(angleY, glm::vec3(0.0f, 1.0f, 0.0f), center);
        }
    }

    void resetObject() {
        for (Submesh* obj : shapes) {
            obj->setTransform(obj->initialTransform);
            obj->color[0] = obj->initialColor[0];
            obj->color[1] = obj->initialColor[1];
            obj->color[2] = obj->initialColor[2];
            obj->updateColor(); // Update VBO with initial color
            obj->showBoundingBox = false; // Hide individual bounding boxes
        }
        moveFullObjectMode = false;
        delete globalBoundingBox;
        globalBoundingBox = nullptr;
    }

private:
    float oldScale = 1.0;

    void updateCenter(){
        vector<Vertex> allVertices;

        for (Submesh* shape : shapes) {
            vector<Vertex> worldVertex = shape->getWorldVertices();
            allVertices.insert(allVertices.end(), worldVertex.begin(), worldVertex.end());
        }
        
        auto [_a,_b,_center] = makeBoundingBox(allVertices);
        center = _center;
    }

    void centerShape(LoadedObject obj)
    {
        if (shapes.empty() || obj.vertices.empty()) {
            return;
        }

        auto [minBound,maxBound,_center] = makeBoundingBox(obj.vertices);

        glm::vec3 size = maxBound - minBound;
        float maxDim = max({size.x, size.y, size.z});
        float scaleFactor = 1.0f / maxDim;

        glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), -center);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor));
        glm::mat4 toScene = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
        
        glm::mat4 normalizationMatrix = toScene * scale * toOrigin;

        center = glm::vec3(normalizationMatrix * glm::vec4(_center,1.0f));

        for (Submesh* shape : shapes) {
            shape->setTransform(normalizationMatrix);
        }
    }
};
