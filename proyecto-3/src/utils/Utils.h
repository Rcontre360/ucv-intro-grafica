#ifndef GL_UTILS_H
#define GL_UTILS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

class Vertex {
public:
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 texCoords;
    glm::vec3 tangent; // Required for Bump/Normal mapping

    static vector<float> flatten(const vector<Vertex>& vertices) {
        vector<float> flatVertices;
        flatVertices.reserve(vertices.size() * 14); // 3+3+3+2+3 = 14 floats
        for (const auto& vertex : vertices) {
            flatVertices.push_back(vertex.position.x);
            flatVertices.push_back(vertex.position.y);
            flatVertices.push_back(vertex.position.z);
            flatVertices.push_back(vertex.normal.x);
            flatVertices.push_back(vertex.normal.y);
            flatVertices.push_back(vertex.normal.z);
            flatVertices.push_back(vertex.color.x);
            flatVertices.push_back(vertex.color.g);
            flatVertices.push_back(vertex.color.b);
            flatVertices.push_back(vertex.texCoords.x);
            flatVertices.push_back(vertex.texCoords.y);
            flatVertices.push_back(vertex.tangent.x);
            flatVertices.push_back(vertex.tangent.y);
            flatVertices.push_back(vertex.tangent.z);
        }
        return flatVertices;
    }
};
            
struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
    glm::vec3 center;
};

// Internal helper for uniform caching
inline GLint getCachedUniformLocation(GLuint program, const string& name) {
    static unordered_map<GLuint, unordered_map<string, GLint>> cache;
    auto& programCache = cache[program];
    auto it = programCache.find(name);
    if (it != programCache.end()) {
        return it->second;
    }
    GLint location = glGetUniformLocation(program, name.c_str());
    programCache[name] = location;
    return location;
}

// Macro to define setGpuVariable overloads
#define DEFINE_SET_GPU_VAR(Type, GlCall) \
inline void setGpuVariable(GLuint program, const string& name, Type value) { \
    GLint location = getCachedUniformLocation(program, name); \
    if (location != -1) { GlCall; } \
}

DEFINE_SET_GPU_VAR(const glm::mat4&, glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value)))
DEFINE_SET_GPU_VAR(const glm::vec3&, glUniform3fv(location, 1, glm::value_ptr(value)))
DEFINE_SET_GPU_VAR(float,             glUniform1f(location, value))
DEFINE_SET_GPU_VAR(int,               glUniform1i(location, value))
DEFINE_SET_GPU_VAR(bool,              glUniform1i(location, static_cast<int>(value)))

#undef DEFINE_SET_GPU_VAR

#endif 
