#pragma once

#include <fstream> // Required for std::ofstream
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
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
        //algorithm is O(n) since for each submesh we run over the vertices only once
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

        bool hasNormals = !info.normals.empty();
        vector<glm::vec3> calculatedNormals;
        if (!hasNormals || true) {
            calculatedNormals = calculateNormals(info, _shapes);
        }

        for (const auto& shape : _shapes) {
            Submesh* newShape = processObject(shape, info, materials, basedir, hasNormals, calculatedNormals);
            if (newShape) {
                loadedObject.shapes.push_back(newShape);
                loadedObject.vertices.insert(loadedObject.vertices.end(), newShape->vertices.begin(), newShape->vertices.end());
            }
        }
        return loadedObject;
    }

    static void saveObject(const std::string& path, const std::vector<Submesh*>& shapes) {
        if (shapes.empty()) {
            throw std::runtime_error("No shapes to save.");
        }

        string objPath = path;
        string mtlPath = path.substr(0, path.find_last_of('.')) + ".mtl";
        string mtlFileName = mtlPath.substr(mtlPath.find_last_of("/\\") + 1);

        ofstream objFile(objPath);
        ofstream mtlFile(mtlPath);

        if (!objFile.is_open()) {
            throw std::runtime_error("Could not open OBJ file for writing: " + objPath);
        }
        if (!mtlFile.is_open()) {
            throw std::runtime_error("Could not open MTL file for writing: " + mtlPath);
        }

        objFile << "# Generated by C3DViewer\n";
        objFile << "mtllib " << mtlFileName << "\n\n";

        mtlFile << "# Generated by C3DViewer\n";

        size_t globalVertexOffset = 0;
        size_t globalNormalOffset = 0;

        for (size_t i = 0; i < shapes.size(); ++i) {
            Submesh* submesh = shapes[i];
            string materialName = "material_" + to_string(i);

            // Write MTL material
            mtlFile << "newmtl " << materialName << "\n";
            mtlFile << "Kd " << submesh->color[0] << " " << submesh->color[1] << " " << submesh->color[2] << "\n\n";

            // Write OBJ object/group and material
            objFile << "o submesh_" << i << "\n";
            objFile << "usemtl " << materialName << "\n";

            vector<Vertex> worldVertices = submesh->getWorldVertices(); // Vertices in world space

            // Write vertices
            for (const auto& vertex : worldVertices) {
                objFile << "v " << vertex.position.x << " " << vertex.position.y << " " << vertex.position.z << "\n";
            }
            // Write normals
            for (const auto& vertex : worldVertices) {
                objFile << "vn " << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << "\n";
            }

            // Write faces (assuming triangles, 3 vertices per face)
            // Each face in Submesh.vertices corresponds to 3 sequential vertices
            // in the flattened list.
            // worldVertices count is vertices.size()
            for (size_t v_idx = 0; v_idx < worldVertices.size(); v_idx += 3) {
                // OBJ indices are 1-based
                // Face format: f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
                // This project does not use texture coordinates (vt)
                objFile << "f "
                        << (globalVertexOffset + v_idx + 1) << "//" << (globalNormalOffset + v_idx + 1) << " "
                        << (globalVertexOffset + v_idx + 2) << "//" << (globalNormalOffset + v_idx + 2) << " "
                        << (globalVertexOffset + v_idx + 3) << "//" << (globalNormalOffset + v_idx + 3) << "\n";
            }
            objFile << "\n";

            globalVertexOffset += worldVertices.size();
            globalNormalOffset += worldVertices.size();
        }

        objFile.close();
        mtlFile.close();
    }
};
