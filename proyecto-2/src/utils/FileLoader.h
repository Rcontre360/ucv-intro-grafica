#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include "tinyobjloader.h"

#include "../submesh/Submesh.h"

using namespace std;

struct LoadedObject {
    std::vector<Submesh*> shapes;
    tinyobj::attrib_t attrib;
};

class FileLoader {
private:
    static vector<glm::vec3> calculateNormals(const tinyobj::attrib_t& info, const vector<tinyobj::shape_t>& shapes) {
        vector<glm::vec3> calculatedNormals;
        calculatedNormals.resize(info.vertices.size() / 3, glm::vec3(0.0f));
        for (const auto& shape : shapes) {
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
                int fv = shape.mesh.num_face_vertices[f];
                if (fv != 3) continue;

                tinyobj::index_t idx0 = shape.mesh.indices[f * 3 + 0];
                tinyobj::index_t idx1 = shape.mesh.indices[f * 3 + 1];
                tinyobj::index_t idx2 = shape.mesh.indices[f * 3 + 2];

                glm::vec3 v0(info.vertices[3 * idx0.vertex_index + 0], info.vertices[3 * idx0.vertex_index + 1], info.vertices[3 * idx0.vertex_index + 2]);
                glm::vec3 v1(info.vertices[3 * idx1.vertex_index + 0], info.vertices[3 * idx1.vertex_index + 1], info.vertices[3 * idx1.vertex_index + 2]);
                glm::vec3 v2(info.vertices[3 * idx2.vertex_index + 0], info.vertices[3 * idx2.vertex_index + 1], info.vertices[3 * idx2.vertex_index + 2]);

                glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

                calculatedNormals[idx0.vertex_index] += faceNormal;
                calculatedNormals[idx1.vertex_index] += faceNormal;
                calculatedNormals[idx2.vertex_index] += faceNormal;
            }
        }
        for (auto& normal : calculatedNormals) {
            normal = glm::normalize(normal);
        }
        return calculatedNormals;
    }

    static Submesh* processObject(const tinyobj::shape_t& shape, const tinyobj::attrib_t& info, const vector<tinyobj::material_t>& materials, const string& basedir, bool hasNormals, const vector<glm::vec3>& calculatedNormals) {
        vector<Vertex> vertices;
        size_t indexOffset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                Vertex vertex;
                
                vertex.position = {
                    info.vertices[3 * idx.vertex_index + 0],
                    info.vertices[3 * idx.vertex_index + 1],
                    info.vertices[3 * idx.vertex_index + 2]
                };

                if (hasNormals) {
                    vertex.normal = {
                        info.normals[3 * idx.normal_index + 0],
                        info.normals[3 * idx.normal_index + 1],
                        info.normals[3 * idx.normal_index + 2]
                    };
                } else {
                    vertex.normal = calculatedNormals[idx.vertex_index];
                }
                
                int materialId = shape.mesh.material_ids[f];
                if (materialId < 0 || materials.empty()) {
                    vertex.color = {0.7f, 0.7f, 0.7f};
                } else {
                    vertex.color = {
                        materials[materialId].diffuse[0],
                        materials[materialId].diffuse[1],
                        materials[materialId].diffuse[2]
                    };
                }

                vertices.push_back(vertex);
            }
            indexOffset += fv;
        }

        if (!vertices.empty()) {
            return new Submesh(vertices);
        }
        return nullptr;
    }

public:
    static LoadedObject loadObject(const std::string& path) {
        LoadedObject loadedObject;
        tinyobj::attrib_t info;
        vector<tinyobj::shape_t> _shapes;
        vector<tinyobj::material_t> materials;
        string warn, err;
        string basedir = path.substr(0, path.find_last_of("/\\") + 1);

        if (!tinyobj::LoadObj(&info, &_shapes, &materials, &warn, &err, path.c_str(), basedir.c_str())) {
            throw std::runtime_error(warn + err);
        }

        loadedObject.attrib = info;

        bool hasNormals = !info.normals.empty();
        vector<glm::vec3> calculatedNormals;
        if (!hasNormals) {
            calculatedNormals = calculateNormals(info, _shapes);
        }

        for (const auto& shape : _shapes) {
            Submesh* newShape = processObject(shape, info, materials, basedir, hasNormals, calculatedNormals);
            if (newShape) {
                loadedObject.shapes.push_back(newShape);
            }
        }
        return loadedObject;
    }
};
