#pragma once

#include "Animation.h"
#include <glm/glm.hpp>
#include <cmath>

class ScaleAnimation : public Animation {
public:
    ScaleAnimation(float durationSecs, float minScale, float maxScale) 
        : Animation(durationSecs) 
    {
        // hardcoded since trees are the only ones scaled
        float yShiftMin = minScale < 1 ? -0.7 : 0.1;
        float yShiftMax = maxScale < 1 ? -0.7 : 0.1;

        addKeyframe(TransformState(glm::vec3(0.0f, yShiftMax, 0.0f), glm::vec3(0.0f), glm::vec3(maxScale)));
        addKeyframe(TransformState(glm::vec3(0.0f, yShiftMin, 0.0f), glm::vec3(0.0f), glm::vec3(minScale)));
    }
};
