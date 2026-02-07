#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include "tinyobjloader.h"
#include "../submesh/Submesh.h"

using namespace std;

struct LoadedObject {
    vector<Submesh*> shapes;
    vector<Vertex> vertices;
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

    static vector<Vertex> extractVertices(const tinyobj::shape_t& shape, const tinyobj::attrib_t& info, const vector<tinyobj::material_t>& materials, bool hasNormals, const vector<glm::vec3>& calculatedNormals) {
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

                if (hasNormals && idx.normal_index >= 0) {
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
        return vertices;
    }

    // we had a bug with some shapes where scaling wasnt working bc their center was far away from ours. Somehow not even with
    // transformation matrixes I was able to fix it so I decided to just move all the vertex of the shape to their correct position (center at 0)
    static void normalizeVertexGroups(vector<vector<Vertex>>& allVertexGroups) {
        glm::vec3 minBound(numeric_limits<float>::max());
        glm::vec3 maxBound(numeric_limits<float>::lowest());
        bool hasData = false;

        for (const auto& group : allVertexGroups) {
            for (const auto& v : group) {
                minBound = glm::min(minBound, v.position);
                maxBound = glm::max(maxBound, v.position);
                hasData = true;
            }
        }

        if (!hasData) return;

        glm::vec3 globalCenter = (minBound + maxBound) * 0.5f;

        for (auto& group : allVertexGroups) {
            for (auto& v : group) {
                v.position -= globalCenter;
            }
        }
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

        bool hasNormals = !info.normals.empty();
        vector<glm::vec3> calculatedNormals = calculateNormals(info, _shapes);

        // Stage A: Extract all vertices first
        vector<vector<Vertex>> allVertexGroups;
        for (const auto& shape : _shapes) {
            vector<Vertex> group = extractVertices(shape, info, materials, hasNormals, calculatedNormals);
            if (!group.empty()) {
                allVertexGroups.push_back(group);
            }
        }

        normalizeVertexGroups(allVertexGroups);

        for (const auto& centeredGroup : allVertexGroups) {
            Submesh* newShape = new Submesh(centeredGroup);
            loadedObject.shapes.push_back(newShape);
            loadedObject.vertices.insert(loadedObject.vertices.end(), centeredGroup.begin(), centeredGroup.end());
        }

        return loadedObject;
    }

    static void saveObject(const std::string& path, const std::vector<Submesh*>& shapes) {
        if (shapes.empty()) throw std::runtime_error("No shapes to save.");

        string objPath = path;
        string mtlPath = path.substr(0, path.find_last_of('.')) + ".mtl";
        string mtlFileName = mtlPath.substr(mtlPath.find_last_of("/\\") + 1);

        ofstream objFile(objPath);
        ofstream mtlFile(mtlPath);

        if (!objFile.is_open() || !mtlFile.is_open()) throw std::runtime_error("Could not open files for writing.");

        objFile << "# Generated by C3DViewer\nmtllib " << mtlFileName << "\n\n";
        mtlFile << "# Generated by C3DViewer\n";

        size_t globalVertexOffset = 0;
        for (size_t i = 0; i < shapes.size(); ++i) {
            Submesh* submesh = shapes[i];
            string matName = "material_" + to_string(i);

            mtlFile << "newmtl " << matName << "\nKd " << submesh->color[0] << " " << submesh->color[1] << " " << submesh->color[2] << "\n\n";
            objFile << "o submesh_" << i << "\nusemtl " << matName << "\n";

            vector<Vertex> worldVertices = submesh->getWorldVertices();
            for (const auto& v : worldVertices) objFile << "v " << v.position.x << " " << v.position.y << " " << v.position.z << "\n";
            for (const auto& v : worldVertices) objFile << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";

            for (size_t v_idx = 0; v_idx < worldVertices.size(); v_idx += 3) {
                objFile << "f " << (globalVertexOffset + v_idx + 1) << "//" << (globalVertexOffset + v_idx + 1) << " "
                        << (globalVertexOffset + v_idx + 2) << "//" << (globalVertexOffset + v_idx + 2) << " "
                        << (globalVertexOffset + v_idx + 3) << "//" << (globalVertexOffset + v_idx + 3) << "\n";
            }
            globalVertexOffset += worldVertices.size();
        }
    }
};
