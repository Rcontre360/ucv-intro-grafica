#pragma once

#include <glm/glm.hpp>
#include "Sphere.h"
#include "../utils/Shaders.h"

enum ShadingMode {
    PHONG = 0,
    BLINN_PHONG = 1,
    FLAT = 2
};

class Light : public Sphere {
private:
    string uPosName, uAmbName, uDiffName, uSpecName, uEnabledName, uModeName;

public:
    int id;
    glm::vec3 ambient  = glm::vec3(0.1f);
    glm::vec3 diffuse  = glm::vec3(0.9f);
    glm::vec3 specular = glm::vec3(0.5f);
    bool enabled = true;
    float animationSpeed = 1.0f;
    float intensity = 1.5f;
    ShadingMode shadingMode = PHONG;

    Light(int _id, glm::vec3 pos) : Sphere(0.15f, pos) {
        id = _id;
        name = "light_" + to_string(id);

        string base = "lights[" + to_string(id) + "].";
        uPosName     = base + "position";
        uAmbName     = base + "ambient";
        uDiffName    = base + "diffuse";
        uSpecName    = base + "specular";
        uEnabledName = base + "enabled";
        uModeName    = base + "shadingMode";
    }

    TransformState getLightTransform(double currentTime) {
        if (animation) return animation->getTransformAt(currentTime * animationSpeed);
        return TransformState();
    }

    void draw(const DrawConfig& config) override {
        TransformState state = getLightTransform(config.currentTime);
        glm::vec3 currentPos = center + state.translation;

        setGpuVariable(config.shaderProgram, uPosName,     currentPos);
        setGpuVariable(config.shaderProgram, uAmbName,     ambient  * intensity);
        setGpuVariable(config.shaderProgram, uDiffName,    diffuse  * intensity);
        setGpuVariable(config.shaderProgram, uSpecName,    specular * intensity);
        setGpuVariable(config.shaderProgram, uEnabledName, enabled);
        setGpuVariable(config.shaderProgram, uModeName,    (int)shadingMode);

        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor,     true);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasDiffuseMap, false);
        glm::vec3 orbColor = enabled ? (diffuse * min(intensity, 2.0f)) : glm::vec3(0.2f);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_color, orbColor);

        for (Submesh* sm : submeshes) {
            sm->setTranslate(currentPos);
            sm->setRotateEuler(state.rotation);
            sm->setScale(state.scale);
            sm->draw(config);
        }
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, false);
    }
};
