#pragma once

#include "Animation.h"

class RotateAnimation : public Animation {
public:
    RotateAnimation(float durationSecs, bool clockwise = true) 
        : Animation(durationSecs) 
    {
        float sign = clockwise ? -1.0f : 1.0f;

        addKeyframe(TransformState(glm::vec3(0.0f), glm::vec3(0.0f, 120.0f * sign, 0.0f), glm::vec3(1.0f)));
        addKeyframe(TransformState(glm::vec3(0.0f), glm::vec3(0.0f, 240.0f * sign, 0.0f), glm::vec3(1.0f)));
        addKeyframe(TransformState(glm::vec3(0.0f), glm::vec3(0.0f, 360.0f * sign, 0.0f), glm::vec3(1.0f)));
    }
};
