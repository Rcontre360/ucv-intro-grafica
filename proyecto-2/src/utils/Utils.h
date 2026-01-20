#ifndef GL_UTILS_H
#define GL_UTILS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>

class Vertex {
public:
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 texCoord;

    static std::vector<float> flatten(const std::vector<Vertex>& vertices) {
        std::vector<float> flatVertices;
        flatVertices.reserve(vertices.size() * 11);
        for (const auto& vertex : vertices) {
            flatVertices.push_back(vertex.position.x);
            flatVertices.push_back(vertex.position.y);
            flatVertices.push_back(vertex.position.z);
            flatVertices.push_back(vertex.color.x);
            flatVertices.push_back(vertex.color.y);
            flatVertices.push_back(vertex.color.z);
            flatVertices.push_back(vertex.texCoord.x);
            flatVertices.push_back(vertex.texCoord.y);
        }
        return flatVertices;
    }
};

void setGpuVariable(GLuint program, const std::string& name, const glm::mat4& value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void setGpuVariable(GLuint program, const std::string& name, const glm::vec3& value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
}

void setGpuVariable(GLuint program, const std::string& name, const glm::vec4& value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform4fv(location, 1, glm::value_ptr(value));
    }
}

void setGpuVariable(GLuint program, const std::string& name, float value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void setGpuVariable(GLuint program, const std::string& name, int value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void setGpuVariable(GLuint program, const std::string& name, bool value) {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location != -1) {
        glUniform1i(location, static_cast<int>(value));
    }
}

#endif // GL_UTILS_H
