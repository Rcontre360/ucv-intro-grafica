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
private:
    // Pre-calculated uniform names to avoid string allocations every frame
    std::string uPosName, uAmbName, uDiffName, uSpecName, uEnabledName, uModeName;

public:
    int id;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float uiColor[3] = { 0.9f, 0.9f, 0.9f };
    bool enabled = true;
    float animationSpeed = 1.0f;
    float intensity = 1.5f; 
    ShadingMode shadingMode = PHONG;

    Light(int _id, glm::vec3 _pos)
    {
        id = _id;
        center = _pos;
        
        // Pre-calculate strings
        std::string base = "lights[" + std::to_string(id) + "].";
        uPosName = base + "position";
        uAmbName = base + "ambient";
        uDiffName = base + "diffuse";
        uSpecName = base + "specular";
        uEnabledName = base + "enabled";
        uModeName = base + "shadingMode";

        setColor(glm::vec3(uiColor[0], uiColor[1], uiColor[2])); 
        
        static vector<Vertex> orbVertices = setupLightOrbMesh();
        Submesh* orb = new Submesh(orbVertices);
        orb->setTranslate(_pos);
        submeshes.push_back(orb);
    }

    void setColor(const glm::vec3& _color) {
        uiColor[0] = _color.r; uiColor[1] = _color.g; uiColor[2] = _color.b;
        diffuse = _color;
        ambient = _color * 0.12f; // Increased by 20% (0.1 -> 0.12)
        specular = glm::vec3(1.0f) * 0.5f;
    }

    TransformState getLightTransform(double currentTime) {
        if (animation) return animation->getTransformAt(currentTime * animationSpeed);
        return TransformState();
    }

    void draw(const DrawConfig& config) override {
        TransformState state = getLightTransform(config.currentTime);
        glm::vec3 currentPos = center + state.translation;

        // 1. Set uniforms using pre-calculated strings
        setGpuVariable(config.shaderProgram, uPosName, currentPos);
        setGpuVariable(config.shaderProgram, uAmbName, ambient * intensity);
        setGpuVariable(config.shaderProgram, uDiffName, diffuse * intensity);
        setGpuVariable(config.shaderProgram, uSpecName, specular * intensity);
        setGpuVariable(config.shaderProgram, uEnabledName, enabled);
        setGpuVariable(config.shaderProgram, uModeName, (int)shadingMode);

        // 2. Draw the debug orb
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, true);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasDiffuseMap, false);
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
        struct TempVertex { glm::vec3 pos; glm::vec2 uv; };
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
                    Vertex v; const auto& tv = temp_verts[lat][lon];
                    v.position = tv.pos * radius; v.normal = tv.pos;
                    v.color = glm::vec3(1.0f); v.texCoords = tv.uv;
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
