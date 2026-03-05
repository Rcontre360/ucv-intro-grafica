#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <GLFW/glfw3.h>

#include "tinyobjloader.h"
#include "../submesh/Object.h"
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

    vector<Object3D*> objects;
    int selectedObjectIndex = 0; // Currently selected object for transformations

    glm::vec3 uiScale = glm::vec3(1.0f);

    State(){}

    ~State()
    {
        for (Object3D* obj : objects) {
            delete obj;
        }
        objects.clear();
    }

    void draw(const DrawConfig& config)
    {
        for (size_t i = 0; i < objects.size(); ++i) {
            objects[i]->draw(config);
        }
    }

    void loadObject(const string& path)
    {
        Object3D* newObj = FileLoader::loadObject(path);
        
        // Center the newly loaded object locally to the camera's front slightly
        glm::vec3 moveTo = Camera::getInstance().initObjectPos;
        glm::vec3 size = newObj->getBoundingBox().max - newObj->getBoundingBox().min;
        float maxDim = std::max({size.x, size.y, size.z});
        float globalScaleFactor = 1.0f / maxDim;

        newObj->setTranslate(moveTo - newObj->center);
        newObj->scale *= globalScaleFactor;
        newObj->updateCenter();

        objects.push_back(newObj);
        selectedObjectIndex = objects.size() - 1; // Auto select new obj
    }

    void rescaleAllShapes(glm::vec3 factor)
    {
        if (selectedObjectIndex >= 0 && selectedObjectIndex < objects.size()) {
            objects[selectedObjectIndex]->setScale(factor);
        }
    }

    void translateFullObject(const glm::vec3& offset)
    {
        if (selectedObjectIndex >= 0 && selectedObjectIndex < objects.size()) {
            objects[selectedObjectIndex]->setTranslate(offset);
        }
    }

    void rotateObject(float angleX, float angleY)
    {
        if (selectedObjectIndex >= 0 && selectedObjectIndex < objects.size()) {
            objects[selectedObjectIndex]->setRotate(angleX, angleY);
        }
    }

    //moves shape to center and scales back to unit cube
    void centerAndScaleBack() {
        if (selectedObjectIndex >= 0 && selectedObjectIndex < objects.size()) {
            Object3D* obj = objects[selectedObjectIndex];
            glm::vec3 moveTo = Camera::getInstance().initObjectPos - obj->center;
            glm::vec3 size = obj->getBoundingBox().max - obj->getBoundingBox().min;

            float maxDim = std::max({size.x, size.y, size.z});
            float globalScaleFactor = 1.0f / maxDim;

            obj->setTranslate(moveTo);
            obj->scale *= globalScaleFactor;
            obj->updateCenter();
        }
    }

    void resetState(){
        showFPS = false;
        enableBackfaceCulling = false;
        enableDepthTest = true;

        uiScale.x = 1.0f;
        uiScale.y = 1.0f;
        uiScale.z = 1.0f;
    }
};
