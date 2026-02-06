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
#include "../utils/Camera.h" // Added this line to include Camera.h

using namespace std;

class State
{
public:
    bool showVertices = false;
    bool showWireframe = false;
    bool showFill = true;
    bool lineAntialiasing = true;
    bool showNormals = false;
    bool moveFullObjectMode = false;
    bool shouldUpdateCenter = false;
    bool showFPS = true; // New
    bool enableBackfaceCulling = false; // New
    bool enableDepthTest = true; // New

    float vertexColor[3] = { 1.0f, 1.0f, 1.0f };
    float pointSize = 5.0f;
    float wireframeColor[3] = { 1.0f, 1.0f, 1.0f };
    float globalBoundingBoxColor[3] = { 0.0f, 1.0f, 0.0f };
    float normalColor[3] = { 1.0f, 1.0f, 0.0f };
    float backgroundColor[3] = { 0.3f, 0.3f, 0.3f }; // New

    vector<Submesh*> shapes;

    glm::vec3 center = glm::vec3(1.0f);
    glm::vec3 oldScale = glm::vec3(1.0f);

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

        vector<Vertex> allVertices;
        for (Submesh* shape : shapes) {
            vector<Vertex> worldVertex = shape->getWorldVertices();
            allVertices.insert(allVertices.end(), worldVertex.begin(), worldVertex.end());
        }
        
        auto [minBound,maxBound,_center] = makeBoundingBox(allVertices);
        center = _center;

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

    void unselectAll(){
        for (Submesh* shape : shapes) {
            shape->showBoundingBox = false;     
        }
    }

    void draw(GLuint shaderProgram)
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
        updateGlobalBoundingBox();

        for (Submesh* obj : shapes) {
            obj->initialTransform = obj->getTransform();
            obj->initialColor[0] = obj->color[0];
            obj->initialColor[1] = obj->color[1];
            obj->initialColor[2] = obj->color[2];
        }
    }

    void rescaleAllShapes(glm::vec3 factor)
    {
        for (Submesh* obj : shapes) {
            obj->setScale((factor / oldScale));
        }

        if (globalBoundingBox){
            globalBoundingBox->setScale((factor / oldScale));
        }

        oldScale = factor;
    }

    void translateFullObject(const glm::vec3& offset)
    {
        if (shapes.empty()) return;

        for (Submesh* obj : shapes) {
            obj->setTranslate(offset); 
        }
        if (globalBoundingBox)
            globalBoundingBox->setTranslate(offset);
    }

    void translateSubmesh(int index, const glm::vec3& offset)
    {
        shouldUpdateCenter = true;
        if (index >= 0 && index < shapes.size()) {
            shapes[index]->setTranslate(offset);
        }
    }

    void rotateObject(float angleX, float angleY)
    {
        if (shapes.empty()) return;

        if (shouldUpdateCenter){
            updateGlobalBoundingBox();
            shouldUpdateCenter = false;
        }

        for (Submesh* obj : shapes) {
            obj->setRotate(angleX, glm::vec3(1.0f, 0.0f, 0.0f), center); 
            obj->setRotate(angleY, glm::vec3(0.0f, 1.0f, 0.0f), center);
        }

        if (globalBoundingBox){
            globalBoundingBox->setRotate(angleX, glm::vec3(1.0f, 0.0f, 0.0f), center); 
            globalBoundingBox->setRotate(angleY, glm::vec3(0.0f, 1.0f, 0.0f), center);
        }
    }

    void centerObjectToInitialPosition() {
        if (shapes.empty()) return;

        updateGlobalBoundingBox(); // Ensure current center is up-to-date

        // Calculate the vector from the current center to the initial object position
        glm::vec3 translationVector = Camera::getInstance().initObjectPos - center;

        // Apply this translation to all submeshes
        for (Submesh* obj : shapes) {
            obj->setTranslate(translationVector);
        }
        // Update global bounding box center as well
        if (globalBoundingBox) {
            globalBoundingBox->setTranslate(translationVector);
        }
        center += translationVector; // Update the stored center
    }

    void resetObject() {
        for (Submesh* obj : shapes) {
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
    void centerShape(LoadedObject obj)
    {
        if (shapes.empty() || obj.vertices.empty()) {
            return;
        }

        auto [minBound,maxBound,_center] = makeBoundingBox(obj.vertices);

        glm::vec3 size = maxBound - minBound;
        float maxDim = max({size.x, size.y, size.z});
        float scaleFactor = 1.0f / maxDim;

        glm::vec3 initialPos = Camera::getInstance().initObjectPos;
        glm::vec3 translation = initialPos - (_center * scaleFactor);

        for (Submesh* shape : shapes) {
            shape->resetTransform();

            shape->scale = glm::vec3(scaleFactor);
            shape->translate = glm::translate(glm::mat4(1.0f), translation);
        }
    }
};
