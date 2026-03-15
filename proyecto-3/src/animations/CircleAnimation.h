#pragma once

#include "Animation.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

class CircleAnimation : public Animation {
    glm::vec3 centerOffset;
    float planeAngleDeg;
    bool clockwise;
    glm::mat4 planeTilt;

public:
    CircleAnimation(glm::vec3 _centerOffset, float _durationSecs, float _planeAngleDeg = 0.0f, bool _clockwise = true) 
        : Animation(_durationSecs) 
    {
        centerOffset = _centerOffset;
        planeAngleDeg = _planeAngleDeg;
        clockwise = _clockwise;
        planeTilt = glm::rotate(glm::mat4(1.0f), glm::radians(planeAngleDeg), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    TransformState getTransformAt(double currentTime) override {
        float t = getNormalizedTime(currentTime);
        float angle = t * 2.0f * glm::pi<float>();
        if (clockwise) angle = -angle;

        glm::vec3 pos = calculateOrbitalPosition(angle);
        glm::vec3 rot = calculateOrbitalRotation(angle);

        return TransformState(pos, rot, glm::vec3(1.0f));
    }

private:
    glm::vec3 calculateOrbitalPosition(float angle) {
        glm::vec3 posRelativeToCenter = -centerOffset;
        glm::mat4 rotMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
        
        glm::vec3 rotatedPos = glm::vec3(rotMatrix * glm::vec4(posRelativeToCenter, 1.0f));
        glm::vec3 posOnPlane = centerOffset + rotatedPos;
        
        return glm::vec3(planeTilt * glm::vec4(posOnPlane, 1.0f));
    }

    glm::vec3 calculateOrbitalRotation(float angle) {
        return glm::vec3(planeAngleDeg, glm::degrees(angle), 0.0f);
    }
};
