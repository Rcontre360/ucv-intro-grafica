#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "../utils/Utils.h"
#include "../animations/Animation.h"

enum ShadingMode {
    PHONG = 0,
    BLINN_PHONG = 1,
    FLAT = 2
};

struct Light {
    glm::vec3 position;
    glm::vec3 initialPosition;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    
    bool enabled = true;
    float animationSpeed = 1.0f;
    ShadingMode shadingMode = PHONG;
    
    Animation* animation = nullptr;

    Light(glm::vec3 pos)
        : initialPosition(pos), position(pos), 
          ambient(0.4f), diffuse(0.9f), specular(1.0f) {}

    ~Light() {
        if (animation) delete animation;
    }

    void update(double currentTime) {
        if (animation) {
            TransformState state = animation->getTransformAt(currentTime * animationSpeed);
            position = initialPosition + state.translation;
        }
    }
};
