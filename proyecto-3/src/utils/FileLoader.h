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
#include "stb/stb_image.h"

using namespace std;

struct LoadedObject {
    vector<Submesh*> shapes;
    vector<Vertex> vertices;
};

class FileLoader {
private:
    static GLuint loadTexture(const string& path) {
        if (path.empty()) return 0;
        
        int width, height, nrComponents;
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            return textureID;
        } else {
            cerr << "Texture failed to load at path: " << path << endl;
            return 0;
        }
    }

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

                if (!info.texcoords.empty() && idx.texcoord_index >= 0) {
                    vertex.texCoords = {
                        info.texcoords[2 * idx.texcoord_index + 0],
                        info.texcoords[2 * idx.texcoord_index + 1]
                    };
                } else {
                    vertex.texCoords = {0.0f, 0.0f};
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
        vector<int> materialIds;
        for (const auto& shape : _shapes) {
            vector<Vertex> group = extractVertices(shape, info, materials, hasNormals, calculatedNormals);
            if (!group.empty()) {
                allVertexGroups.push_back(group);
                materialIds.push_back(shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[0]);
            }
        }

        normalizeVertexGroups(allVertexGroups);
        
        for (size_t i = 0; i < allVertexGroups.size(); ++i) {
            Submesh* newShape = new Submesh(allVertexGroups[i]);
            int matId = materialIds[i];
            if (matId >= 0 && matId < materials.size()) {
                const auto& mat = materials[matId];
                if (!mat.diffuse_texname.empty()) newShape->diffuseMap = loadTexture(basedir + mat.diffuse_texname);
                if (!mat.specular_texname.empty()) newShape->specularMap = loadTexture(basedir + mat.specular_texname);
                if (!mat.bump_texname.empty()) newShape->normalMap = loadTexture(basedir + mat.bump_texname);
                if (!mat.ambient_texname.empty()) newShape->ambientMap = loadTexture(basedir + mat.ambient_texname);
            }
            loadedObject.shapes.push_back(newShape);
            loadedObject.vertices.insert(loadedObject.vertices.end(), allVertexGroups[i].begin(), allVertexGroups[i].end());
        }

        return loadedObject;
    }
};