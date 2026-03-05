#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Submesh.h"
#include "../utils/Utils.h"

using namespace std;

class Object3D
{
public:
    vector<Submesh*> shapes;
    glm::vec3 center = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::mat4 translate = glm::mat4(1.0f);
    glm::quat rotate = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    glm::vec3 oldScale = glm::vec3(1.0f);

    Object3D(vector<Submesh*> shapes) : shapes(shapes) {
        updateCenter();
    }

    ~Object3D()
    {
        for (Submesh* obj : shapes) {
            delete obj;
        }
        shapes.clear();
    }

    void draw(const DrawConfig& config)
    {
        for (size_t i = 0; i < shapes.size(); ++i) {
            glm::mat4 objTransform = getTransform();
            glm::mat4 combinedModel = objTransform * shapes[i]->getTransform();

            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::model, combinedModel);

            if (shapes[i]->diffuseMap) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, shapes[i]->diffuseMap);
                setGpuVariable(config.shaderProgram, Shaders::DefaultShader::diffuseMap, 0);
                setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasDiffuseMap, true);
            } else {
                setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasDiffuseMap, false);
            }
            
            glBindVertexArray(shapes[i]->vao);
            glDrawArrays(GL_TRIANGLES, 0, shapes[i]->vertexCount);
            glBindVertexArray(0);
        }
    }

    void setTranslate(const glm::vec3& offset) { 
        translate = glm::translate(glm::mat4(1.0f), offset) * translate;
        center += offset;
    }

    void setScale(const glm::vec3& factor) { 
        scale *= (factor / oldScale);
        oldScale = factor;
    }

    void setRotate(float angleX, float angleY) { 
        setRotate(angleY, glm::vec3(0.0f, 1.0f, 0.0f));
        setRotate(angleX, glm::vec3(1.0f, 0.0f, 0.0f)); 
    }

    void setRotate(float angle, const glm::vec3& axis) { 
        // rotate over the object's center
        glm::quat rotationQuad = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
        rotate = rotationQuad * rotate;

        // get the vector from center to the current position
        glm::vec3 pos = glm::vec3(translate[3]);
        glm::vec3 dirToPivot = pos - center;

        // obtain new position and calculate the vector towards it
        glm::vec3 newDir = rotationQuad * dirToPivot;
        glm::vec3 delta = (center + newDir) - pos;

        // apply given translation
        setTranslate(delta);
    }

    const glm::mat4 getTransform() const { 
        glm::mat4 R = glm::mat4(rotate);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        return translate * R * S;
    }

    BoundingBox getBoundingBox(){
        vector<Vertex> allVertices;
        for (Submesh* shape : shapes) {
            vector<Vertex> worldVertex = shape->getWorldVertices();
            allVertices.insert(allVertices.end(), worldVertex.begin(), worldVertex.end());
        }
        return makeBoundingBox(allVertices);
    }

    void updateCenter() {
        if (!shapes.empty()) {
            auto [minBound, maxBound, _center] = getBoundingBox();
            center = _center;
        }
    }
};
