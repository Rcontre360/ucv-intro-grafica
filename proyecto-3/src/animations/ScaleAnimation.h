#pragma once

#include "Animation.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

class ScaleAnimation : public Animation {
    float minScale;
    float maxScale;
    float yShiftMin;
    float yShiftMax;

public:
    ScaleAnimation(float _durationSecs, float _minScale, float _maxScale) 
        : Animation(_durationSecs) 
    {
        minScale = _minScale;
        maxScale = _maxScale;
        yShiftMin = minScale < 1 ? -0.7f : 0.1f;
        yShiftMax = maxScale < 1 ? -0.7f : 0.1f;
    }

    TransformState getTransformAt(double currentTime) override {
        float t = getNormalizedTime(currentTime);
        
        float wave = (sin(t * 2.0f * glm::pi<float>() - glm::pi<float>() / 2.0f) + 1.0f) / 2.0f;

        float currentScale = glm::mix(minScale, maxScale, wave);
        float currentY = glm::mix(yShiftMin, yShiftMax, wave);

        return TransformState(glm::vec3(0.0f, currentY, 0.0f), glm::vec3(0.0f), glm::vec3(currentScale));
    }
};
