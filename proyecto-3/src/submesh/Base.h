#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <algorithm>
#include <limits>
#include "../utils/Utils.h"
#include "../utils/Camera.h"
#include "../utils/Shaders.h"

using namespace std;

struct DrawConfig {
    GLuint shaderProgram;
    double currentTime = 0.0;
};

class BaseSubmesh 
{
public:
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::quat rotate = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::mat4 translate = glm::mat4(1.0f);
    glm::mat4 initialTransform;

    vector<Vertex> vertices;
    float color[3];
    float initialColor[3];

    GLuint vao = 0;
    GLuint vbo = 0;

    GLuint diffuseMap = 0;
    GLuint specularMap = 0;
    GLuint normalMap = 0;
    GLuint ambientMap = 0;

    int vertexCount = 0;

    BaseSubmesh(const vector<Vertex>& vertices) 
        : vertices(vertices), vertexCount(vertices.size())
    {
        if (!vertices.empty()) {
            color[0] = vertices[0].color.r;
            color[1] = vertices[0].color.g;
            color[2] = vertices[0].color.b;
        }

        vector<float> flatVertices = Vertex::flatten(vertices);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, flatVertices.size() * sizeof(float), flatVertices.data(), GL_STATIC_DRAW);
        
        // Position attribute (location 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal attribute (location 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Color attribute (location 2)
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // TexCoord attribute (location 3)
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
    }

    virtual ~BaseSubmesh()
    {
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        if (diffuseMap) glDeleteTextures(1, &diffuseMap);
        if (specularMap) glDeleteTextures(1, &specularMap);
        if (normalMap) glDeleteTextures(1, &normalMap);
        if (ambientMap) glDeleteTextures(1, &ambientMap);
    }

    // Draws the submesh using the provided shader program.
    virtual void draw(const DrawConfig& config)
    {
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::model, getTransform());

        if (diffuseMap) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseMap);
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::diffuseMap, 0);
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasDiffuseMap, true);
        } else {
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasDiffuseMap, false);
        }
        
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }

    // gets vertex object in the world. Basically apply transformations to the vertex we have on cpu
    vector<Vertex> getWorldVertices() const {
        glm::mat4 transformMat = getTransform();

        vector<Vertex> worldV;
        worldV.reserve(vertices.size());

        for (const auto& v : vertices) {
            Vertex res = v;
            glm::vec4 worldPos = transformMat * glm::vec4(v.position, 1.0f);
            res.position = glm::vec3(worldPos);

            worldV.push_back(res);
        }

        return worldV;
    }

    // translate
    void setTranslate(const glm::vec3& offset) { 
        translate = glm::translate(glm::mat4(1.0f), offset);
    }

    // normal rotate
    void setRotate(float angle, const glm::vec3& axis) { 
        rotate = glm::angleAxis(glm::radians(angle), axis);
    }

    // rotate over a given center.
    // used bc submeshes might be far from shape center and must rotate around it
    void setRotate(float angle, const glm::vec3& axis, const glm::vec3& center) { 
        // Absolute rotation
        rotate = glm::angleAxis(glm::radians(angle), glm::normalize(axis));

        // For absolute translation around a pivot, we need the base relative position.
        // Since this is an absolute setter, we assume we calculate the new translation 
        // purely based on the given rotation and pivot point.
        glm::vec3 pos = glm::vec3(initialTransform[3]); // Use original position
        glm::vec3 dirToPivot = pos - center;

        glm::vec3 newDir = rotate * dirToPivot;
        glm::vec3 delta = (center + newDir) - pos;

        // Set absolute translation based on this delta plus whatever the original translation was
        translate = glm::translate(glm::mat4(1.0f), glm::vec3(initialTransform[3]) + delta);
    }

    void setScale(const glm::vec3& factor) { 
        scale = factor;
    }

    void setRotateEuler(const glm::vec3& eulerDegrees) {
        glm::quat qx = glm::angleAxis(glm::radians(eulerDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat qy = glm::angleAxis(glm::radians(eulerDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat qz = glm::angleAxis(glm::radians(eulerDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
        rotate = qz * qy * qx;
    }

    void updateColor() {
        for (auto& vertex : vertices) {
            vertex.color = {color[0], color[1], color[2]};
        }
        vector<float> flatVertices = Vertex::flatten(vertices);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, flatVertices.size() * sizeof(float), flatVertices.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    const glm::mat4 getTransform() const { 
        glm::mat4 R = glm::mat4(rotate);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        return translate * R * S;
    }

    void resetTransform() { 
        translate = glm::mat4(1.0f); 
        rotate = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); 
        scale = glm::vec3(1.0f); 
    }
};
