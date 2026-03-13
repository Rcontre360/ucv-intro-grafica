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
#include "../objects/BumpSphere.h"

using namespace std;

class State
{
public:
    bool showFPS = true;
    bool enableBackfaceCulling = false;
    bool enableDepthTest = true;
    bool enableFatt = true;

    float backgroundColor[3] = { 0.1f, 0.1f, 0.1f };

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
        // Light 1: Red-ish
        Light* l1 = new Light(0, glm::vec3(-2.0f, 5.0f, -2.0f));
        l1->setColor(glm::vec3(1.0f, 0.196f, 0.098f)); 
        l1->animation = new CircleAnimation(glm::vec3(-3.0f, -3.0f, -3.0f), 7.0f, 0.0f, true);
        lights.push_back(l1);

        // Light 2: Green-ish
        Light* l2 = new Light(1, glm::vec3(0.0f, 5.0f, 0.0f));
        l2->setColor(glm::vec3(0.196f, 1.0f, 0.098f)); 
        l2->animation = new CircleAnimation(glm::vec3(-2.0f, -2.0f, -2.0f), 7.0f, 0.0f, false);
        lights.push_back(l2);

        // Light 3: Blue-ish
        Light* l3 = new Light(2, glm::vec3(2.0f, 5.0f, 2.0f));
        l3->setColor(glm::vec3(0.098f, 0.196f, 1.0f)); 
        l3->animation = new CircleAnimation(glm::vec3(-3.0f, -3.0f, -3.0f), 7.0f, 0.0f, false);
        lights.push_back(l3);
    }

    void update(double currentTime) {
    }

    void draw(const DrawConfig& config)
    {
        glUseProgram(config.shaderProgram);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::viewPos, Camera::getInstance().position);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uEnableFatt, enableFatt);

        // Draw lights first (this sets the uniforms for other objects)
        for (Light* l : lights) {
            l->draw(config);
        }

        // Draw normal objects
        for (Object* obj : objects) {
            obj->draw(config);
            if (obj->isSelected) {
                drawBoundingBox(config, obj->getBoundingBox());
            }
        }
    }

    void drawBoundingBox(const DrawConfig& config, const BoundingBox& box) {
        static GLuint vao = 0, vbo = 0;
        if (vao == 0) {
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
        }

        glm::vec3 v[8] = {
            {box.min.x, box.min.y, box.min.z}, {box.max.x, box.min.y, box.min.z},
            {box.max.x, box.max.y, box.min.z}, {box.min.x, box.max.y, box.min.z},
            {box.min.x, box.min.y, box.max.z}, {box.max.x, box.min.y, box.max.z},
            {box.max.x, box.max.y, box.max.z}, {box.min.x, box.max.y, box.max.z}
        };

        unsigned int indices[] = {
            0, 1, 1, 2, 2, 3, 3, 0, // Bottom
            4, 5, 5, 6, 6, 7, 7, 4, // Top
            0, 4, 1, 5, 2, 6, 3, 7  // Verticals
        };

        vector<float> data;
        for (int i = 0; i < 24; ++i) {
            data.push_back(v[indices[i]].x);
            data.push_back(v[indices[i]].y);
            data.push_back(v[indices[i]].z);
        }

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::model, glm::mat4(1.0f));
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, true);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_color, glm::vec3(0.0f, 1.0f, 0.0f));

        glDrawArrays(GL_LINES, 0, 24);

        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, false);
        glBindVertexArray(0);
    }

    void loadScene(const string& path)
    {
        for (auto obj : objects) delete obj;
        objects.clear();

        // 1. Load objects from the OBJ file
        LoadedScene sceneData = FileLoader::loadScene(path);
        for (const auto& objData : sceneData.objects) {
            Object* newObj = new Object();
            newObj->name = objData.name;
            newObj->localBox = objData.localBox;
            
            for (const auto& smData : objData.submeshes) {
                Submesh* sm = new Submesh(smData.vertices);
                sm->diffuseMap = smData.diffuseMap;
                sm->specularMap = smData.specularMap;
                sm->normalMap = smData.normalMap;
                sm->ambientMap = smData.ambientMap;
                sm->resetTransform();
                sm->initialTransform = sm->getTransform();
                newObj->submeshes.push_back(sm);
            }

            newObj->center = newObj->getBoundingBox().center;
            for (Submesh* sm : newObj->submeshes) {
                sm->pivot = newObj->center;
            }
            objects.push_back(newObj);
        }

        // 2. Add the Parametric Bump Sphere (Requirement 5.b)
        BumpSphere* bumpSphere = new BumpSphere(0.4f, 32, 32);
        bumpSphere->name = "parametric_bump_sphere";
        bumpSphere->translate(glm::vec3(0.0f, 0.5f, 0.0f)); // center of table area
        objects.push_back(bumpSphere);
        
        initializeAnimations();

        cout << "Loaded Scene: " << path << endl;
        cout << "Total Objects: " << objects.size() << endl;
        for (const auto& obj : objects) {
            cout << " - [Object] " << obj->name << " | Submeshes: " << obj->submeshes.size() << endl;
            for (size_t i = 0; i < obj->submeshes.size(); ++i) {
                Submesh* sm = obj->submeshes[i];
                if (sm->diffuseMap)  cout << "    > Submesh " << i << ": Found Diffuse Map" << endl;
                if (sm->ambientMap)  cout << "    > Submesh " << i << ": Found Ambient (AO) Map" << endl;
                if (sm->specularMap) cout << "    > Submesh " << i << ": Found Specular Map" << endl;
                if (sm->normalMap)   cout << "    > Submesh " << i << ": Found Normal (Bump) Map" << endl;
            }
        }
        cout << "------------------------------------------" << endl;
    }

    void initializeAnimations() {
        for (Object* obj : objects) {
            if (obj->name == "santa_flight") {
                obj->setAnimation(new CircleAnimation(glm::vec3(0.0f, 0.0f, 20.0f), 10.0f, 0.0f, true));
            }
            if (obj->name == "santa_ground") {
                obj->setAnimation(new CircleAnimation(glm::vec3(-2.0f, 0.0f, 0.0f), 10.0f, 0.0f, true));
            }
            if (obj->name.rfind("snowman_red_", 0) == 0) {
                int id = std::stoi(obj->name.substr(12)); 
                obj->setAnimation(new RotateAnimation(2.0f + id * 0.2f, id % 2 == 0));
            }
            if (obj->name.rfind("snowman_blue_", 0) == 0) {
                int id = std::stoi(obj->name.substr(13)); 
                obj->setAnimation(new RotateAnimation(2.0f + id * 0.2f, id % 2 == 0));
            }
            if (obj->name.rfind("pine_", 0) == 0) {
                int id = std::stoi(obj->name.substr(5)); 
                if (id >= 3 && id <= 6) {
                    obj->setAnimation(new ScaleAnimation(5.0f + id * 0.3f, 0.5f, 1.0f));
                }
            }
            // Apply reflectivity
            if (obj->name == "bauble_core") {
                for (auto sm : obj->submeshes) sm->reflectivity = 0.6f;
            }
        }
    }
};
