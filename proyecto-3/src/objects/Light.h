#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <string>
#include <vector>
#include "Object.h"
#include "Submesh.h"
#include "../animations/Animation.h"
#include "../utils/Shaders.h"

enum ShadingMode {
    PHONG = 0,
    BLINN_PHONG = 1,
    FLAT = 2
};

class Light : public Object {
public:
    int id;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    
    bool enabled = true;
    float animationSpeed = 1.0f;
    float intensity = 1.5f; // Initial intensity
    ShadingMode shadingMode = PHONG;

    Light(int _id, glm::vec3 _pos)
    {
        id = _id;
        center = _pos;
        setColor(glm::vec3(0.9f)); 
        
        static vector<Vertex> orbVertices = setupLightOrbMesh();
        Submesh* orb = new Submesh(orbVertices);
        orb->setTranslate(_pos);
        submeshes.push_back(orb);
    }

    void setColor(const glm::vec3& _color) {
        diffuse = _color;
        ambient = _color * 0.1f;
        specular = glm::vec3(1.0f) * 0.5f;
    }

    TransformState getLightTransform(double currentTime) {
        if (animation) {
            return animation->getTransformAt(currentTime * animationSpeed);
        }
        return TransformState();
    }

    void draw(const DrawConfig& config) override {
        TransformState state = getLightTransform(config.currentTime);
        glm::vec3 currentPos = center + state.translation;

        // 1. Set uniforms for the shader to use this light for OTHER objects
        string base = "lights[" + to_string(id) + "].";
        setGpuVariable(config.shaderProgram, base + "position", currentPos);
        setGpuVariable(config.shaderProgram, base + "ambient", ambient * intensity);
        setGpuVariable(config.shaderProgram, base + "diffuse", diffuse * intensity);
        setGpuVariable(config.shaderProgram, base + "specular", specular * intensity);
        setGpuVariable(config.shaderProgram, base + "enabled", enabled);
        setGpuVariable(config.shaderProgram, base + "shadingMode", (int)shadingMode);

        // 2. Draw the debug orb (emissive)
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, true);
        // The orb itself looks brighter as intensity increases
        glm::vec3 orbColor = enabled ? (diffuse * std::min(intensity, 2.0f)) : glm::vec3(0.2f);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_color, orbColor);
        
        for (Submesh* sm : submeshes) {
            sm->setTranslate(currentPos);
            sm->setRotateEuler(state.rotation);
            sm->setScale(state.scale);
            sm->draw(config);
        }
        
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, false);
    }

private:
    static vector<Vertex> setupLightOrbMesh() {
        const unsigned int X_SEGMENTS = 16;
        const unsigned int Y_SEGMENTS = 16;
        const float radius = 0.15f;
        vector<Vertex> lightOrbVertices;

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
                float xPos = std::cos(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());
                float yPos = std::cos(ySegment * glm::pi<float>());
                float zPos = std::sin(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());

                row.push_back({glm::vec3(xPos, yPos, zPos), glm::vec2(xSegment, ySegment)});
            }
            temp_verts.push_back(row);
        }

        for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
            for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
                auto createVertex = [&](unsigned int lat, unsigned int lon) {
                    Vertex v;
                    const auto& tv = temp_verts[lat][lon];
                    v.position = tv.pos * radius;
                    v.normal = tv.pos;
                    v.color = glm::vec3(1.0f);
                    v.texCoords = tv.uv;
                    return v;
                };

                lightOrbVertices.push_back(createVertex(y, x));
                lightOrbVertices.push_back(createVertex(y + 1, x));
                lightOrbVertices.push_back(createVertex(y + 1, x + 1));

                lightOrbVertices.push_back(createVertex(y, x));
                lightOrbVertices.push_back(createVertex(y + 1, x + 1));
                lightOrbVertices.push_back(createVertex(y, x + 1));
            }
        }
        return lightOrbVertices;
    }
};
