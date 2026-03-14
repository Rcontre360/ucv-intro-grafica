#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <cmath>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include "tinyobjloader.h"
#include "../objects/Object.h"
#include "../utils/FileLoader.h"
#include "../utils/Utils.h"
#include "../utils/Camera.h"
#include "../animations/CircleAnimation.h"
#include "../animations/RotateAnimation.h"
#include "../animations/ScaleAnimation.h"
#include "../objects/Light.h"
#include "../objects/Sphere.h"

using namespace std;

class State
{
public:
    vector<Object*> objects;
    vector<Light*> lights;

    State(){
        initializeLights();
    }

    ~State()
    {
        for (Object* obj : objects) delete obj;
        objects.clear();
        for (Light* l : lights) delete l;
        lights.clear();
    }

    void initializeLights() {
        // red 
        Light* l1 = new Light(0, glm::vec3(-1.0f, 0.5f, -1.0f));
        l1->diffuse  = glm::vec3(1.0f, 0.196f, 0.098f);
        l1->ambient  = glm::vec3(0.3f);
        l1->specular = glm::vec3(0.5f);
        // circle  
        l1->animation = new CircleAnimation(glm::vec3(-3.0f, -3.0f, -1.0f), 7.0f, 0.0f, true);
        lights.push_back(l1);

        // green
        Light* l2 = new Light(1, glm::vec3(-1.0f, 2.0f, -1.0f));
        l2->diffuse  = glm::vec3(0.196f, 1.0f, 0.098f);
        l2->ambient  = glm::vec3(0.3f);
        l2->specular = glm::vec3(0.5f);
        // circle 
        l2->animation = new CircleAnimation(glm::vec3(3.0f, 0.0f, -5.0f), 7.0f, 0.0f, false);
        lights.push_back(l2);

        // blue 
        Light* l3 = new Light(2, glm::vec3(-4.5f, 1.0f, 2.0f));
        l3->diffuse  = glm::vec3(0.098f, 0.196f, 1.0f);
        l3->ambient  = glm::vec3(0.3f);
        l3->specular = glm::vec3(0.5f);
        // circle
        l3->animation = new CircleAnimation(glm::vec3(5.0f, -3.0f, -3.0f), 7.0f, 0.0f, false);
        lights.push_back(l3);
    }

    void draw(const DrawConfig& config)
    {
        glUseProgram(config.shaderProgram);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::viewPos, Camera::getInstance().position);

        for (Light* l : lights) l->draw(config);
        for (Object* obj : objects) obj->draw(config);
    }

    void loadScene(const string& path)
    {
        vector<ObjectData> sceneData = FileLoader::loadScene(path);
        createFromData(sceneData);
    }

    void createFromData(vector<ObjectData>& sceneData) {
        for (auto& objData : sceneData) {
            Object* newObj = new Object();
            newObj->name = objData.name;
            newObj->localBox = objData.localBox;

            for (auto& smData : objData.submeshes) {
                Submesh* sm = new Submesh(smData.vertices);
                sm->diffuseMap  = FileLoader::uploadTexture(smData.diffuseMap);
                smData.diffuseMap.free();
                sm->specularMap = FileLoader::uploadTexture(smData.specularMap);
                smData.specularMap.free();
                sm->normalMap   = FileLoader::uploadTexture(smData.normalMap);
                smData.normalMap.free();
                sm->ambientMap  = FileLoader::uploadTexture(smData.ambientMap);
                smData.ambientMap.free();

                sm->resetTransform();
                sm->initialTransform = sm->getTransform();
                newObj->submeshes.push_back(sm);
            }

            newObj->center = newObj->getBoundingBox().center;
            for (Submesh* sm : newObj->submeshes)
                sm->pivot = newObj->center;
            objects.push_back(newObj);
        }

        Sphere* sphere = new Sphere(1.2f, glm::vec3(-3.0f, 0.5f, -2.5f));
        sphere->name = "parametric_sphere";
        objects.push_back(sphere);

        initializeAnimations();
    }

    void initializeAnimations() {
        for (Object* obj : objects) {
            if (obj->name == "santa_flight")
                obj->setAnimation(new CircleAnimation(glm::vec3(0.0f, 0.0f, 20.0f), 10.0f, 0.0f, true));
            if (obj->name == "santa_ground")
                obj->setAnimation(new CircleAnimation(glm::vec3(-2.0f, 0.0f, 0.0f), 10.0f, 0.0f, true));
            if (obj->name.rfind("snowman_red_", 0) == 0) {
                int id = stoi(obj->name.substr(12));
                obj->setAnimation(new RotateAnimation(2.0f + id * 0.2f, id % 2 == 0));
            }
            if (obj->name.rfind("snowman_blue_", 0) == 0) {
                int id = stoi(obj->name.substr(13));
                obj->setAnimation(new RotateAnimation(2.0f + id * 0.2f, id % 2 == 0));
            }
            if (obj->name.rfind("pine_", 0) == 0) {
                int id = stoi(obj->name.substr(5));
                if (id >= 3 && id <= 6)
                    obj->setAnimation(new ScaleAnimation(5.0f + id * 0.3f, 0.5f, 1.0f));
            }
            if (obj->name == "bauble_core") {
                for (auto sm : obj->submeshes) sm->reflectivity = 0.6f;
            }
        }
    }
};
