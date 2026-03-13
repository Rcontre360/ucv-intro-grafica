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

struct SubmeshData;

struct DrawConfig {
    GLuint shaderProgram;
    double currentTime = 0.0;
    GLuint skyboxTextureID = 0; 
};

class Submesh 
{
public:
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::quat rotate = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::mat4 translate = glm::mat4(1.0f);
    glm::mat4 initialTransform;
    glm::vec3 pivot = glm::vec3(0.0f);

    vector<Vertex> vertices;
    float color[3];

    GLuint vao = 0;
    GLuint vbo = 0;

    GLuint diffuseMap = 0;
    GLuint specularMap = 0;
    GLuint normalMap = 0;
    GLuint ambientMap = 0;

    int vertexCount = 0;
    float reflectivity = 0.0f; 
    float shininess = 16.0f; // Lower power = larger highlights
    int sMappingMode = 0; // 0: Standard, 1: Spherical, 2: Cylindrical
    int oMappingMode = 0; // 0: Position, 1: Normal

    Submesh(const vector<Vertex>& vertices) 
        : vertices(vertices), vertexCount(vertices.size())
    {
        setupGL();
    }

    void setupGL() {
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
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal attribute (location 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Color attribute (location 2)
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // TexCoord attribute (location 3)
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(3);

        // Tangent attribute (location 4)
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
        glEnableVertexAttribArray(4);

        glBindVertexArray(0);
    }

    virtual ~Submesh()
    {
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        if (diffuseMap) glDeleteTextures(1, &diffuseMap);
        if (specularMap) glDeleteTextures(1, &specularMap);
        if (normalMap) glDeleteTextures(1, &normalMap);
        if (ambientMap) glDeleteTextures(1, &ambientMap);
    }

    virtual void draw(const DrawConfig& config)
    {
        glm::mat4 model = getTransform();
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::model, model);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uReflectivity, reflectivity);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uShininess, shininess);

        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uSMappingMode, sMappingMode);
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uOMappingMode, oMappingMode);
        
        // Transform local pivot to world space to get the true center
        glm::vec3 worldCenter = glm::vec3(model * glm::vec4(pivot, 1.0f));
        setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uObjCenter, worldCenter);

        if (reflectivity > 0.0f && config.skyboxTextureID != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, config.skyboxTextureID);
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::skybox, 1);
        }

        if (diffuseMap) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseMap);
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::diffuseMap, 0);
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasDiffuseMap, true);
        } else {
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasDiffuseMap, false);
        }

        if (normalMap) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, normalMap);
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::normalMap, 2);
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasNormalMap, true);
        } else {
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasNormalMap, false);
        }

        if (specularMap) {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, specularMap);
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::specularMap, 3);
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasSpecularMap, true);
        } else {
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasSpecularMap, false);
        }

        if (ambientMap) {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, ambientMap);
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::ambientMap, 4);
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasAmbientMap, true);
        } else {
            setGpuVariable(config.shaderProgram, Shaders::DefaultShader::uHasAmbientMap, false);
        }
        
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }

    void setTranslate(const glm::vec3& offset) { 
        translate = glm::translate(glm::mat4(1.0f), offset);
    }

    void setRotate(float angle, const glm::vec3& axis) { 
        rotate = glm::angleAxis(glm::radians(angle), axis);
    }

    void setRotate(float angle, const glm::vec3& axis, const glm::vec3& center) { 
        rotate = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
        glm::vec3 pos = glm::vec3(initialTransform[3]); 
        glm::vec3 dirToPivot = pos - center;
        glm::vec3 newDir = rotate * dirToPivot;
        glm::vec3 delta = (center + newDir) - pos;
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

    const glm::mat4 getTransform() const { 
        glm::mat4 R = glm::mat4(rotate);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        
        glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), -pivot);
        glm::mat4 fromOrigin = glm::translate(glm::mat4(1.0f), pivot);

        return translate * fromOrigin * R * S * toOrigin;
    }

    void resetTransform() { 
        translate = glm::mat4(1.0f); 
        rotate = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); 
        scale = glm::vec3(1.0f); 
    }
};
