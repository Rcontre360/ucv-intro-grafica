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
#include "../lights/Light.h"

using namespace std;

class State
{
public:
    bool showFPS = true;
    bool enableBackfaceCulling = false;
    bool enableDepthTest = true;
    bool enableFatt = true;

    float normalColor[3] = { 1.0f, 1.0f, 0.0f };
    float backgroundColor[3] = { 0.1f, 0.1f, 0.1f };

    vector<Object*> objects;
    vector<Light*> lights;
    BaseSubmesh* lightOrbMesh = nullptr;
    GLuint axisVAO = 0, axisVBO = 0;

    State(){
        initializeLights();
        setupLightOrbMesh();
        setupAxes();
    }

    ~State()
    {
        for (Object* obj : objects) {
            delete obj;
        }
        objects.clear();
        for (Light* light : lights) {
            delete light;
        }
        lights.clear();
        if (lightOrbMesh) delete lightOrbMesh;
        if (axisVAO) glDeleteVertexArrays(1, &axisVAO);
        if (axisVBO) glDeleteBuffers(1, &axisVBO);
    }

    void setupAxes() {
        float axesVertices[] = {
            -100.0f, 0.0f, 0.0f,
             100.0f, 0.0f, 0.0f,
             0.0f, 0.0f, -100.0f,
             0.0f, 0.0f,  100.0f
        };
        glGenVertexArrays(1, &axisVAO);
        glGenBuffers(1, &axisVBO);
        glBindVertexArray(axisVAO);
        glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(axesVertices), axesVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void setupLightOrbMesh() {
        // Simple cube vertices for the light orbs
        float s = 0.1f; // small size
        vector<glm::vec3> p = {
            {-s,-s,-s}, {s,-s,-s}, {s,s,-s}, {-s,s,-s},
            {-s,-s,s}, {s,-s,s}, {s,s,s}, {-s,s,s}
        };
        vector<int> indices = {
            0,1,2, 0,2,3, 4,5,6, 4,6,7, 0,4,7, 0,7,3,
            1,5,6, 1,6,2, 0,1,5, 0,5,4, 3,2,6, 3,6,7
        };
        vector<Vertex> verts;
        for(int idx : indices) {
            Vertex v;
            v.position = p[idx];
            v.normal = glm::normalize(p[idx]);
            v.color = glm::vec3(1.0f);
            v.texCoords = glm::vec2(0.0f);
            verts.push_back(v);
        }
        lightOrbMesh = new BaseSubmesh(verts);
    }

    void initializeLights() {
        Light* l1 = new Light(glm::vec3(-2.0f, 5.0f, -2.0f));
        l1->animation = new CircleAnimation(glm::vec3(-3.0f, -3.0f, -3.0f), 7.0f, 0.0f, true);
        lights.push_back(l1);

        Light* l2 = new Light(glm::vec3(0.0f, 5.0f, 0.0f));
        l2->animation = new CircleAnimation(glm::vec3(-2.0f, -2.0f, -2.0f), 7.0f, 0.0f, false);
        lights.push_back(l2);

        Light* l3 = new Light(glm::vec3(2.0f, 5.0f, 2.0f));
        l3->animation = new CircleAnimation(glm::vec3(-3.0f, -3.0f, -3.0f), 7.0f, 0.0f, false);
        lights.push_back(l3);
    }

    void update(double currentTime) {
        for (auto light : lights) {
            light->update(currentTime);
        }
    }

    void draw(const DrawConfig& config)
    {
        glUseProgram(config.shaderProgram);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::viewPos, Camera::getInstance().position);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uEnableFatt, enableFatt);

        for (int i = 0; i < (int)lights.size(); i++) {
            Light* l = lights[i];
            // Cache locations if not already found
            if (l->cachedLocs.pos == -1) {
                string base = "lights[" + to_string(i) + "].";
                l->cachedLocs.pos = glGetUniformLocation(config.shaderProgram, (base + "position").c_str());
                l->cachedLocs.amb = glGetUniformLocation(config.shaderProgram, (base + "ambient").c_str());
                l->cachedLocs.diff = glGetUniformLocation(config.shaderProgram, (base + "diffuse").c_str());
                l->cachedLocs.spec = glGetUniformLocation(config.shaderProgram, (base + "specular").c_str());
                l->cachedLocs.enabled = glGetUniformLocation(config.shaderProgram, (base + "enabled").c_str());
                l->cachedLocs.mode = glGetUniformLocation(config.shaderProgram, (base + "shadingMode").c_str());
            }

            // Use cached locations directly
            if (l->cachedLocs.pos != -1) glUniform3fv(l->cachedLocs.pos, 1, glm::value_ptr(l->position));
            if (l->cachedLocs.amb != -1) glUniform3fv(l->cachedLocs.amb, 1, glm::value_ptr(l->ambient));
            if (l->cachedLocs.diff != -1) glUniform3fv(l->cachedLocs.diff, 1, glm::value_ptr(l->diffuse));
            if (l->cachedLocs.spec != -1) glUniform3fv(l->cachedLocs.spec, 1, glm::value_ptr(l->specular));
            if (l->cachedLocs.enabled != -1) glUniform1i(l->cachedLocs.enabled, l->enabled);
            if (l->cachedLocs.mode != -1) glUniform1i(l->cachedLocs.mode, (int)l->shadingMode);
        }

        for (Object* obj : objects) {
            obj->draw(config);
        }

        // Draw debug spheres for lights
        if (lightOrbMesh) {
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, true);
            for (auto light : lights) {
                if (!light->enabled) continue;
                
                lightOrbMesh->setTranslate(light->position);
                setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_color, light->diffuse);
                lightOrbMesh->draw(config);
            }
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, false);
        }

        // Draw Axes
        if (axisVAO) {
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::model, glm::mat4(1.0f));
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, true);
            glBindVertexArray(axisVAO);
            glLineWidth(2.0f);
            
            // X Axis: Red
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_color, glm::vec3(1.0f, 0.0f, 0.0f));
            glDrawArrays(GL_LINES, 0, 2);
            
            // Z Axis: Blue
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_color, glm::vec3(0.0f, 0.0f, 1.0f));
            glDrawArrays(GL_LINES, 2, 2);
            
            glLineWidth(1.0f);
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, false);
            glBindVertexArray(0);
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
            
            for (size_t i = 0; i < objData.submeshes.size(); ++i) {
                Submesh* sm = new Submesh(objData.submeshes[i]);
                FileLoader::applyMaterials(sm, objData.materialIds[i], sceneData.materials, basedir);
                
                sm->resetTransform();
                sm->initialTransform = sm->getTransform();
                sm->initialColor[0] = sm->color[0];
                sm->initialColor[1] = sm->color[1];
                sm->initialColor[2] = sm->color[2];

                newObj->submeshes.push_back(sm);
            }

            if (!newObj->submeshes.empty()) {
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
                obj->setAnimation(new CircleAnimation(glm::vec3(0.0f, 0.0f, 20.0f), 10.0f, 0.0f, true));
            }
            if (obj->name == "santa_ground") {
                obj->setAnimation(new CircleAnimation(glm::vec3(-3.0f, 0.0f, 0.0f), 10.0f, 0.0f, true));
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
        }
    }

    void resetState(){
        showFPS = false;
        enableBackfaceCulling = false;
        enableDepthTest = true;
    }
};
