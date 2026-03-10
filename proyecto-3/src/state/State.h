#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <cmath>
#include <GLFW/glfw3.h>

#include "tinyobjloader.h"
#include "../objects/Object.h"
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
    Submesh* lightOrbMesh = nullptr;
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
        // Generate a UV sphere for the light orbs
        const unsigned int X_SEGMENTS = 16;
        const unsigned int Y_SEGMENTS = 16;
        const float radius = 0.15f;
        const float PI = 3.14159265359f;
        vector<Vertex> verts;

        struct TempVertex {
            glm::vec3 pos;
            glm::vec2 uv;
        };
        vector<vector<TempVertex>> temp_verts;

        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
            vector<TempVertex> row;
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                row.push_back({glm::vec3(xPos, yPos, zPos), glm::vec2(xSegment, ySegment)});
            }
            temp_verts.push_back(row);
        }

        for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
            for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
                // First triangle
                Vertex v1, v2, v3;
                v1.position = temp_verts[y][x].pos * radius;
                v1.normal = temp_verts[y][x].pos;
                v1.color = glm::vec3(1.0f);
                v1.texCoords = temp_verts[y][x].uv;

                v2.position = temp_verts[y+1][x].pos * radius;
                v2.normal = temp_verts[y+1][x].pos;
                v2.color = glm::vec3(1.0f);
                v2.texCoords = temp_verts[y+1][x].uv;

                v3.position = temp_verts[y+1][x+1].pos * radius;
                v3.normal = temp_verts[y+1][x+1].pos;
                v3.color = glm::vec3(1.0f);
                v3.texCoords = temp_verts[y+1][x+1].uv;

                verts.push_back(v1); verts.push_back(v2); verts.push_back(v3);

                // Second triangle
                Vertex v4, v5, v6;
                v4.position = temp_verts[y][x].pos * radius;
                v4.normal = temp_verts[y][x].pos;
                v4.color = glm::vec3(1.0f);
                v4.texCoords = temp_verts[y][x].uv;

                v5.position = temp_verts[y+1][x+1].pos * radius;
                v5.normal = temp_verts[y+1][x+1].pos;
                v5.color = glm::vec3(1.0f);
                v5.texCoords = temp_verts[y+1][x+1].uv;

                v6.position = temp_verts[y][x+1].pos * radius;
                v6.normal = temp_verts[y][x+1].pos;
                v6.color = glm::vec3(1.0f);
                v6.texCoords = temp_verts[y][x+1].uv;

                verts.push_back(v4); verts.push_back(v5); verts.push_back(v6);
            }
        }
        lightOrbMesh = new Submesh(verts);
    }

    void initializeLights() {
        // Light 1: Red-ish
        Light* l1 = new Light(glm::vec3(-2.0f, 5.0f, -2.0f));
        l1->diffuse = glm::vec3(1.0f, 0.196f, 0.098f); // R=255, G=50, B=25
        l1->ambient = l1->diffuse * 0.1f;
        l1->specular = l1->diffuse;
        l1->animation = new CircleAnimation(glm::vec3(-3.0f, -3.0f, -3.0f), 7.0f, 0.0f, true);
        lights.push_back(l1);

        // Light 2: Green-ish
        Light* l2 = new Light(glm::vec3(0.0f, 5.0f, 0.0f));
        l2->diffuse = glm::vec3(0.196f, 1.0f, 0.098f); // R=50, G=255, B=25
        l2->ambient = l2->diffuse * 0.1f;
        l2->specular = l2->diffuse;
        l2->animation = new CircleAnimation(glm::vec3(-2.0f, -2.0f, -2.0f), 7.0f, 0.0f, false);
        lights.push_back(l2);

        // Light 3: Blue-ish
        Light* l3 = new Light(glm::vec3(2.0f, 5.0f, 2.0f));
        l3->diffuse = glm::vec3(0.098f, 0.196f, 1.0f); // R=25, G=50, B=255
        l3->ambient = l3->diffuse * 0.1f;
        l3->specular = l3->diffuse;
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
            string base = "lights[" + to_string(i) + "].";
            setGpuVariable(config.shaderProgram, base + "position", l->position);
            setGpuVariable(config.shaderProgram, base + "ambient", l->ambient);
            setGpuVariable(config.shaderProgram, base + "diffuse", l->diffuse);
            setGpuVariable(config.shaderProgram, base + "specular", l->specular);
            setGpuVariable(config.shaderProgram, base + "enabled", l->enabled);
            setGpuVariable(config.shaderProgram, base + "shadingMode", (int)l->shadingMode);
        }

        for (Object* obj : objects) {
            obj->draw(config);
        }

        // Draw debug spheres for lights
        if (lightOrbMesh) {
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, true);
            for (auto light : lights) {
                lightOrbMesh->setTranslate(light->position);
                glm::vec3 orbColor = light->enabled ? light->diffuse : glm::vec3(0.2f);
                setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_color, orbColor);
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
