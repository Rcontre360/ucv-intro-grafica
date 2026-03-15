#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Submesh.h"
#include "../utils/Utils.h"
#include "../animations/Animation.h"

class Object {
public:
    string name;
    vector<Submesh*> submeshes;
    glm::vec3 center = glm::vec3(0.0f);
    BoundingBox localBox;
    Animation* animation = nullptr;
    bool isSelected = false;

    Object() {}

    virtual ~Object() {
        for (Submesh* sm : submeshes) delete sm;
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
        for (Submesh* sm : submeshes)
            sm->draw(config);
        if (isSelected)
            drawBoundingBox(config, getBoundingBox());
    }

    void setAnimation(Animation* anim) {
        if (animation) delete animation;
        animation = anim;
    }

    BoundingBox getBoundingBox() const {
        if (submeshes.empty()) return localBox;
        glm::mat4 transform = submeshes[0]->getTransform();
        glm::vec3 minB(numeric_limits<float>::max()), maxB(numeric_limits<float>::lowest());
        for (const auto& c : localBox.corners) {
            glm::vec3 wc = glm::vec3(transform * glm::vec4(c, 1.0f));
            minB = glm::min(minB, wc);
            maxB = glm::max(maxB, wc);
        }
        BoundingBox box;
        box.min = minB; box.max = maxB;
        box.center = (minB + maxB) * 0.5f;
        box.recalcCorners();
        return box;
    }

private:
    void drawBoundingBox(const DrawConfig& config, const BoundingBox& box) {
        static GLuint vao = 0, vbo = 0;
        if (!vao) { glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); }

        static const unsigned int idx[] = {
            0,1, 1,2, 2,3, 3,0,
            4,5, 5,6, 6,7, 7,4,
            0,4, 1,5, 2,6, 3,7
        };

        vector<float> data;
        for (int i = 0; i < 24; ++i) {
            data.push_back(box.corners[idx[i]].x);
            data.push_back(box.corners[idx[i]].y);
            data.push_back(box.corners[idx[i]].z);
        }

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::model, glm::mat4(1.0f));
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, true);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasDiffuseMap, false);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::u_color, glm::vec3(0.0f, 1.0f, 0.0f));

        glDrawArrays(GL_LINES, 0, 24);

        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasColor, false);
        glBindVertexArray(0);
    }
};
