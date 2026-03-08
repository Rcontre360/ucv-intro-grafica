#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <cmath>

struct TransformState {
    glm::vec3 translation;
    glm::vec3 rotation; // Euler angles in degrees (x, y, z)
    glm::vec3 scale;

    TransformState(glm::vec3 t = glm::vec3(0.0f), glm::vec3 r = glm::vec3(0.0f), glm::vec3 s = glm::vec3(1.0f)) 
        : translation(t), rotation(r), scale(s) {}
};

class Animation {
public:
    std::vector<TransformState> keyframes;
    float duration; // Total duration of the animation in seconds

    Animation(float durationSecs = 2.0f) : duration(durationSecs) {}

    void addKeyframe(const TransformState& state) {
        keyframes.push_back(state);
    }

    TransformState getTransformAt(double currentTime) {
        if (keyframes.empty()) return TransformState();
        if (keyframes.size() == 1) return keyframes[0];

        // Normalize time between 0.0 and 1.0 based on duration
        float t = fmod(currentTime, duration) / duration;
        
        int numIntervals = keyframes.size(); 
        
        // Find which keyframe interval we are currently in
        float scaledT = t * numIntervals;
        int currentIndex = (int)scaledT;
        if (currentIndex >= numIntervals) currentIndex = numIntervals - 1;
        int nextIndex = (currentIndex + 1) % numIntervals; // Wraps around to 0 for a smooth loop
        float localT = scaledT - (float)currentIndex; // Factor from 0 to 1 between current and next keyframe

        // Interpolate between the two keyframes
        TransformState result;
        const TransformState& k1 = keyframes[currentIndex];
        const TransformState& k2 = keyframes[nextIndex];

        result.translation = glm::mix(k1.translation, k2.translation, localT);
        
        // Fix rotation wrapping:
        // Instead of pure linear mix, we calculate the difference, wrap it to [-180, 180], 
        // and add it back. This ensures when interpolating from e.g. 240 back to 0, 
        // the difference is 120 (forward to 360) rather than -240 (backward).
        glm::vec3 rotDiff = k2.rotation - k1.rotation;
        for(int i = 0; i < 3; ++i) {
            // Ensure diff is between -180 and 180 degrees to always take shortest continuous path
            rotDiff[i] = fmod(rotDiff[i] + 180.0f, 360.0f);
            if (rotDiff[i] < 0)
                rotDiff[i] += 360.0f;
            rotDiff[i] -= 180.0f;
        }
        result.rotation = k1.rotation + rotDiff * localT;
        
        result.scale = glm::mix(k1.scale, k2.scale, localT);

        return result;
    }
};