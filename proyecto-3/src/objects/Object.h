#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <glm/glm.hpp>
#include "Submesh.h"
#include "../utils/Utils.h"
#include "../utils/Camera.h"
#include "../animations/Animation.h"

class Object {
public:
    std::string name;
    std::vector<Submesh*> submeshes;
    glm::vec3 center = glm::vec3(0.0f);
    glm::vec3 oldScale = glm::vec3(1.0f);
    BoundingBox localBox; 
    Animation* animation = nullptr;
    bool isSelected = false;
    
    Object() {}
    
    virtual ~Object() {
        for (Submesh* sm : submeshes) {
            delete sm;
        }
        submeshes.clear();
        if (animation) delete animation;
    }

    virtual void draw(const DrawConfig& config) {
        if (animation) {
            TransformState state = animation->getTransformAt(config.currentTime);
            for (Submesh* sm : submeshes) {
                sm->setTranslate(state.translation);
                sm->setRotateEuler(state.rotation);
                sm->setScale(state.scale);
            }
        }
        for (Submesh* sm : submeshes) {
            sm->draw(config);
        }
    }

    void setAnimation(Animation* anim) {
        if (animation) {
            delete animation;
        }
        animation = anim;
    }

    // Faster version that transforms the 8 corners of the AABB instead of every vertex
    BoundingBox getBoundingBox() {
        if (submeshes.empty()) return localBox;
        glm::mat4 transform = submeshes[0]->getTransform();
        glm::vec3 minBound(numeric_limits<float>::max());
        glm::vec3 maxBound(numeric_limits<float>::lowest());

        glm::vec3 corners[8] = {
            {localBox.min.x, localBox.min.y, localBox.min.z},
            {localBox.min.x, localBox.min.y, localBox.max.z},
            {localBox.min.x, localBox.max.y, localBox.min.z},
            {localBox.min.x, localBox.max.y, localBox.max.z},
            {localBox.max.x, localBox.min.y, localBox.min.z},
            {localBox.max.x, localBox.min.y, localBox.max.z},
            {localBox.max.x, localBox.max.y, localBox.min.z},
            {localBox.max.x, localBox.max.y, localBox.max.z}
        };

        for (int i = 0; i < 8; ++i) {
            glm::vec3 worldCorner = glm::vec3(transform * glm::vec4(corners[i], 1.0f));
            minBound = glm::min(minBound, worldCorner);
            maxBound = glm::max(maxBound, worldCorner);
        }

        return { minBound, maxBound, (minBound + maxBound) * 0.5f };
    }

    void translate(const glm::vec3& offset) {
        for (Submesh* sm : submeshes) {
            sm->setTranslate(offset);
        }
        center += offset;
    }

    void rotate(float angleX, float angleY) {
        for (Submesh* sm : submeshes) {
            sm->setRotate(angleX, glm::vec3(1.0f, 0.0f, 0.0f), center);
            sm->setRotate(angleY, glm::vec3(0.0f, 1.0f, 0.0f), center);
        }
    }
};
