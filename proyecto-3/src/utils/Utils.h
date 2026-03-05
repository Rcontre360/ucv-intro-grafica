#ifndef GL_UTILS_H
#define GL_UTILS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>

using namespace std;

class Vertex {
public:
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 texCoords;

    static vector<float> flatten(const vector<Vertex>& vertices) {
        vector<float> flatVertices;
        flatVertices.reserve(vertices.size() * 11);
        for (const auto& vertex : vertices) {
            flatVertices.push_back(vertex.position.x);
            flatVertices.push_back(vertex.position.y);
            flatVertices.push_back(vertex.position.z);
            flatVertices.push_back(vertex.normal.x);
            flatVertices.push_back(vertex.normal.y);
            flatVertices.push_back(vertex.normal.z);
            flatVertices.push_back(vertex.color.x);
            flatVertices.push_back(vertex.color.y);
            flatVertices.push_back(vertex.color.z);
            flatVertices.push_back(vertex.texCoords.x);
            flatVertices.push_back(vertex.texCoords.y);
        }
        return flatVertices;
    }
};
            
struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
    glm::vec3 center;
};

BoundingBox makeBoundingBox(const vector<Vertex>& vertices){
    glm::vec3 minBound = glm::vec3(numeric_limits<float>::max());
    glm::vec3 maxBound = glm::vec3(numeric_limits<float>::lowest());
    for (const auto& vertex : vertices) {
        minBound.x = min(minBound.x, vertex.position.x);
        minBound.y = min(minBound.y, vertex.position.y);
        minBound.z = min(minBound.z, vertex.position.z);
        maxBound.x = max(maxBound.x, vertex.position.x);
        maxBound.y = max(maxBound.y, vertex.position.y);
        maxBound.z = max(maxBound.z, vertex.position.z);
    }
    glm::vec3 center = (maxBound + minBound) / 2.0f;

    return { minBound, maxBound, center };
}

void setGpuVariable(GLuint program, const string& name, const glm::mat4& value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void setGpuVariable(GLuint program, const string& name, const glm::mat3& value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void setGpuVariable(GLuint program, const string& name, const glm::vec3& value) {    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
}

void setGpuVariable(GLuint program, const string& name, const glm::vec4& value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform4fv(location, 1, glm::value_ptr(value));
    }
}

void setGpuVariable(GLuint program, const string& name, float value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void setGpuVariable(GLuint program, const string& name, int value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void setGpuVariable(GLuint program, const string& name, bool value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform1i(location, static_cast<int>(value));
    }
}

#endif 
