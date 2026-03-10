#pragma once

#include <glm/glm.hpp>
#include <cmath>

struct TransformState {
    glm::vec3 translation;
    glm::vec3 rotation; 
    glm::vec3 scale;

    TransformState(glm::vec3 _t = glm::vec3(0.0f), glm::vec3 _r = glm::vec3(0.0f), glm::vec3 _s = glm::vec3(1.0f)) 
    {
        translation = _t;
        rotation = _r;
        scale = _s;
    }
};

class Animation {
public:
    float duration; 

    Animation(float _durationSecs = 2.0f) 
    {
        duration = _durationSecs;
    }
    virtual ~Animation() {}

    virtual TransformState getTransformAt(double currentTime) = 0;

protected:
    float getNormalizedTime(double currentTime) {
        if (duration <= 0.0f) return 0.0f;
        return fmod((float)currentTime, duration) / duration;
    }
};
