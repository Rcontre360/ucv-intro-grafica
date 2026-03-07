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
#include "../animations/CircleAnimation.h"
#include "../animations/RotateAnimation.h"
#include "../animations/ScaleAnimation.h"

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

                for (size_t objIdx = 0; objIdx < sceneData.objects.size(); ++objIdx) {
                    const auto& objData = sceneData.objects[objIdx];
                    Object* newObj = new Object();
                    newObj->name = objData.name;
                    newObj->localBox = objData.localBox;
                    
                    cout << "Loaded Object ID: " << objIdx << " | Name: " << objData.name << endl;
        
                    for (size_t i = 0; i < objData.submeshes.size(); ++i) {                Submesh* sm = new Submesh(objData.submeshes[i]);
                FileLoader::applyMaterials(sm, objData.materialIds[i], sceneData.materials, basedir);
                
                sm->resetTransform();
                sm->scale = glm::vec3(1.0f);
                sm->translate = glm::mat4(1.0f);
                
                sm->initialTransform = sm->getTransform();
                sm->initialColor[0] = sm->color[0];
                sm->initialColor[1] = sm->color[1];
                sm->initialColor[2] = sm->color[2];

                newObj->submeshes.push_back(sm);
            }

            if (!newObj->submeshes.empty()) {
                // Now that the scale and translation are set, the world center is correct
                newObj->center = newObj->getBoundingBox().center;
                for (Submesh* sm : newObj->submeshes) {
                    sm->pivot = newObj->center;
                }
                objects.push_back(newObj);
            } else {
                delete newObj;
            }
        }
        
        initializeAnimations();
    }

    void initializeAnimations() {
        for (Object* obj : objects) {
            
            if (obj->name == "santa_flight") {
                float duration = 20.0f;
                glm::vec3 centerOffset(0.0f, 0.0f, 20.0f);
                
                CircleAnimation* anim = new CircleAnimation(centerOffset, duration, 0.0f, true);
                obj->setAnimation(anim);
            }

            if (obj->name == "santa_ground") {
                float duration = 10.0f;
                glm::vec3 centerOffset(-3.0f, 0.0f, 0.0f);
                
                CircleAnimation* anim = new CircleAnimation(centerOffset, duration, 0.0f, true);
                obj->setAnimation(anim);
            }

            if (obj->name.rfind("snowman_red_", 0) == 0) {
                int snowmanId = 1;
                snowmanId = std::stoi(obj->name.substr(12)); 

                bool clockwise = (snowmanId % 2 == 0);
                float duration = 2.0f + (snowmanId * 0.2f); 

                RotateAnimation* anim = new RotateAnimation(duration, clockwise); 
                obj->setAnimation(anim);
            }

            if (obj->name.rfind("snowman_blue_", 0) == 0) {
                int snowmanId = 1;
                snowmanId = std::stoi(obj->name.substr(13)); 

                bool clockwise = (snowmanId % 2 == 0);
                float duration = 2.0f + (snowmanId * 0.5f); 

                RotateAnimation* anim = new RotateAnimation(duration, clockwise); 
                obj->setAnimation(anim);
            }

            if (obj->name.rfind("pine_", 0) == 0) {
                int pineId = 0;
                pineId = std::stoi(obj->name.substr(5)); 

                if (pineId >= 3 && pineId <= 6) {
                    float duration = 5.0f + (pineId * 0.3f); 
                    
                    ScaleAnimation* anim = new ScaleAnimation(duration, 0.5f, 1.0f);
                    obj->setAnimation(anim);
                }
            }
        }
    }

    void resetState(){
        showFPS = false;
        enableBackfaceCulling = false;
        enableDepthTest = true;
    }
};
