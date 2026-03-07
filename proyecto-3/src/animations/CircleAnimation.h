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

        // The starting position of the object relative to the center
        glm::vec3 startPosRelativeToCenter = -centerOffset;

        // Base horizontal plane rotation (tilt around X axis)
        glm::mat4 planeTilt = glm::rotate(glm::mat4(1.0f), glm::radians(planeAngleDeg), glm::vec3(1.0f, 0.0f, 0.0f));

        for (int i = 1; i <= STEPS; ++i) {
            float t = (float)i / STEPS;
            float angle = t * 2.0f * (float)M_PI;

            if (clockwise) {
                // To rotate clockwise, angle moves positively mathematically but we adjust sign
                angle = -angle; 
            }

            // Create a rotation matrix around the Y axis (since plane is X/Z originally)
            glm::mat4 rotMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

            // Rotate the starting position around the center
            glm::vec3 rotatedPosRelativeToCenter = glm::vec3(rotMatrix * glm::vec4(startPosRelativeToCenter, 1.0f));

            // Bring back to local coordinate system where start is (0,0,0)
            glm::vec3 posOnPlane = centerOffset + rotatedPosRelativeToCenter;

            // Apply plane tilt to the position
            glm::vec3 tiltedPos = glm::vec3(planeTilt * glm::vec4(posOnPlane, 1.0f));

            // The object's own rotation (Yaw).
            // We want it to face tangent to the circle. 
            // If the center is directly in front (+Z), it starts facing forward (0 deg).
            float yawAngle = glm::degrees(angle);
            glm::vec3 rot(planeAngleDeg, yawAngle, 0.0f);

            addKeyframe(TransformState(tiltedPos, rot, glm::vec3(1.0f)));
        }
    }
};
