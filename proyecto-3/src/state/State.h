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
#include "../utils/Camera.h" 

using namespace std;

class State
{
public:
    bool showFPS = true;
    bool enableBackfaceCulling = false;
    bool enableDepthTest = true;

    float normalColor[3] = { 1.0f, 1.0f, 0.0f };
    float backgroundColor[3] = { 0.5f, 0.5f, 0.5f };

    vector<Object*> objects;

    State(){}

    ~State()
    {
        for (Object* obj : objects) {
            delete obj;
        }
        objects.clear();
    }

    void draw(const DrawConfig& config)
    {
        for (Object* obj : objects) {
            obj->draw(config);
        }
    }

    void loadScene(const string& path)
    {
        for (Object* obj : objects) delete obj;
        objects.clear();

        LoadedScene sceneData = FileLoader::loadScene(path);
        string basedir = path.substr(0, path.find_last_of("/\\" ) + 1);

        float maxDim = max({
            sceneData.sceneBox.max.x - sceneData.sceneBox.min.x,
            sceneData.sceneBox.max.y - sceneData.sceneBox.min.y,
            sceneData.sceneBox.max.z - sceneData.sceneBox.min.z
        });
        float scaleFactor = 1.0f / maxDim;
        glm::vec3 initialPos = Camera::getInstance().initObjectPos;
        glm::vec3 globalTranslation = initialPos - (sceneData.sceneBox.center * scaleFactor);

        for (const auto& objData : sceneData.objects) {
            Object* newObj = new Object();
            newObj->name = objData.name;
            newObj->localBox = objData.localBox;
            
            for (size_t i = 0; i < objData.submeshes.size(); ++i) {
                Submesh* sm = new Submesh(objData.submeshes[i]);
                FileLoader::applyMaterials(sm, objData.materialIds[i], sceneData.materials, basedir);
                
                sm->resetTransform();
                sm->scale = glm::vec3(scaleFactor);
                sm->translate = glm::translate(glm::mat4(1.0f), globalTranslation);
                
                sm->initialTransform = sm->getTransform();
                sm->initialColor[0] = sm->color[0];
                sm->initialColor[1] = sm->color[1];
                sm->initialColor[2] = sm->color[2];

                newObj->submeshes.push_back(sm);
            }

            if (!newObj->submeshes.empty()) {
                // Now that the scale and translation are set, the world center is correct
                newObj->center = newObj->getBoundingBox().center;
                objects.push_back(newObj);
            } else {
                delete newObj;
            }
        }
    }

    void resetState(){
        showFPS = false;
        enableBackfaceCulling = false;
        enableDepthTest = true;
    }
};