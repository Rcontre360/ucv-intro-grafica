#pragma once

#include <glm/glm.hpp>
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
