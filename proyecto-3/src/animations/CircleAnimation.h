#pragma once

#include "Animation.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

class CircleAnimation : public Animation {
public:
    static const int STEPS = 20;

    // centerOffset: Vector from object's starting position to the center of the circle.
    // planeAngleDeg: 0 means the circle is on the X/Z plane (horizontal). 
    // clockwise: If true, rotates clockwise. If false, rotates counter-clockwise.
    CircleAnimation(glm::vec3 centerOffset, float durationSecs, float planeAngleDeg = 0.0f, bool clockwise = true) 
        : Animation(durationSecs) 
    {
        float radius = glm::length(centerOffset);
        if (radius == 0.0f) return;

        glm::mat4 planeTilt = glm::rotate(glm::mat4(1.0f), glm::radians(planeAngleDeg), glm::vec3(1.0f, 0.0f, 0.0f));

        for (int i = 1; i <= STEPS; ++i) {
            float t = (float)i / STEPS;
            float angle = t * 2.0f * (float)M_PI;

            glm::vec3 posOnPlane;
            glm::vec3 rot;

            if (clockwise) {
                // Starts at (0,0,0). Moves left (X) and forward (Z) to go clockwise.
                posOnPlane.x = -radius * sin(angle);
                posOnPlane.y = 0.0f;
                posOnPlane.z = -radius * cos(angle) + radius;
                rot = glm::vec3(planeAngleDeg, glm::degrees(angle), 0.0f);
            } else {
                // Starts at (0,0,0). Moves right (X) and forward (Z) to go counter-clockwise.
                posOnPlane.x = radius * sin(angle);
                posOnPlane.y = 0.0f;
                posOnPlane.z = -radius * cos(angle) + radius;
                rot = glm::vec3(planeAngleDeg, -glm::degrees(angle), 0.0f);
            }

            glm::vec3 tiltedPos = glm::vec3(planeTilt * glm::vec4(posOnPlane, 1.0f));

            addKeyframe(TransformState(tiltedPos, rot, glm::vec3(1.0f)));
        }
    }
};
