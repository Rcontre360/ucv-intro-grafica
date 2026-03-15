#pragma once

#include "Animation.h"

class RotateAnimation : public Animation {
    bool clockwise;
public:
    RotateAnimation(float _durationSecs, bool _clockwise = true) 
        : Animation(_durationSecs) 
    {
        clockwise = _clockwise;
    }

    TransformState getTransformAt(double currentTime) override {
        float t = getNormalizedTime(currentTime);
        float angle = t * 360.0f * (clockwise ? -1.0f : 1.0f);
        
        return TransformState(glm::vec3(0.0f), glm::vec3(0.0f, angle, 0.0f), glm::vec3(1.0f));
    }
};
