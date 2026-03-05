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
#include "../utils/Camera.h" 

using namespace std;

class State
{
public:
    bool shouldUpdateCenter = false;
    bool showFPS = true; // New
    bool enableBackfaceCulling = false; // New
    bool enableDepthTest = true; // New

    float normalColor[3] = { 1.0f, 1.0f, 0.0f };
    float backgroundColor[3] = { 0.5f, 0.5f, 0.5f }; // New

    vector<Submesh*> shapes;

    glm::vec3 uiScale = glm::vec3(1.0f);
    glm::vec3 center = glm::vec3(0.0f);
    glm::vec3 oldScale = glm::vec3(1.0f);

    State(){}

    ~State()
    {
        for (Submesh* obj : shapes) {
            delete obj;
        }
        shapes.clear();
    }

    BoundingBox getBoundingBox(){
        vector<Vertex> allVertices;
        for (Submesh* shape : shapes) {
            vector<Vertex> worldVertex = shape->getWorldVertices();
            allVertices.insert(allVertices.end(), worldVertex.begin(), worldVertex.end());
        }
        
        return makeBoundingBox(allVertices);
    }

    void draw(const DrawConfig& config)
    {
        for (size_t i = 0; i < shapes.size(); ++i) {
            shapes[i]->draw(config);
        }
    }

    void loadObject(const string& path)
    {
        for (Submesh* obj : shapes) {
            delete obj;
        }
        shapes.clear();
        resetState();

        LoadedObject loaded = FileLoader::loadObject(path);
        shapes = loaded.shapes;

        centerOnLoad(loaded);
        
        if (!shapes.empty()) {
            auto [minBound, maxBound, _center] = getBoundingBox();
            center = _center;
        }

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
        oldScale = factor;
    }

    void translateFullObject(const glm::vec3& offset)
    {
        if (shapes.empty()) return;

        for (Submesh* obj : shapes) {
            obj->setTranslate(offset); 
        }
        center += offset;
    }

    void rotateObject(float angleX, float angleY)
    {
        if (shapes.empty()) return;

        for (Submesh* obj : shapes) {
            obj->setRotate(angleX, glm::vec3(1.0f, 0.0f, 0.0f), center); 
            obj->setRotate(angleY, glm::vec3(0.0f, 1.0f, 0.0f), center);
        }
    }

    //moves shape to center and scales back to unit cube
    void centerAndScaleBack() {
        if (shapes.empty()) return;

        BoundingBox box = getBoundingBox();
        glm::vec3 moveTo = Camera::getInstance().initObjectPos - center;
        glm::vec3 size = box.max - box.min;

        float maxDim = std::max({size.x, size.y, size.z});
        float globalScaleFactor = 1.0f / maxDim;

        for (Submesh* obj : shapes) {
            obj->setTranslate(moveTo);
            obj->scale *= globalScaleFactor;
        }
        center += moveTo; 
    }

    void resetState(){
        showFPS = false;
        enableBackfaceCulling = false;
        enableDepthTest = true;

        uiScale.x = 1.0f;
        uiScale.y = 1.0f;
        uiScale.z = 1.0f;
        oldScale.x = 1.0f;
        oldScale.y = 1.0f;
        oldScale.z = 1.0f;
    }

private:
    void centerOnLoad(LoadedObject obj)
    {
        if (shapes.empty() || obj.vertices.empty()) {
            return;
        }

        BoundingBox box = makeBoundingBox(obj.vertices);
        centerShape(box);
    }

    void centerShape(BoundingBox box){
        glm::vec3 size = box.max - box.min;
        float maxDim = max({size.x, size.y, size.z});
        float scaleFactor = 1.0f / maxDim;

        glm::vec3 initialPos = Camera::getInstance().initObjectPos;
        glm::vec3 translation = initialPos - (box.center * scaleFactor);

        for (Submesh* shape : shapes) {
            shape->resetTransform();

            shape->scale = glm::vec3(scaleFactor);
            shape->translate = glm::translate(glm::mat4(1.0f), translation);
        }
    }
};