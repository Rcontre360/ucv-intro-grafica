#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include "Submesh.h"
#include "tinyobjloader.h"
#include "stb/stb_image.h"

using namespace std;

struct LoadedObject {
    std::vector<Submesh*> shapes;
    tinyobj::attrib_t attrib;
};

class FileLoader {
public:
    static GLuint loadTexture(const std::string& path) {
        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format;
            if (nrChannels == 1) format = GL_RED;
            else if (nrChannels == 3) format = GL_RGB;
            else if (nrChannels == 4) format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            std::cerr << "Failed to load texture: " << path << std::endl;
        }
        stbi_image_free(data);

        return textureId;
    }

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
            calculatedNormals.resize(info.vertices.size() / 3, glm::vec3(0.0f));
            for (const auto& shape : _shapes) {
                for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
                    int fv = shape.mesh.num_face_vertices[f];
                    if (fv != 3) continue; // Only process triangles

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
        }

        for (const auto& shape : _shapes) {
            vector<float> vertices;
            int vertexCount = 0;
            size_t indexOffset = 0;
            GLuint textureId = 0;

            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                int fv = shape.mesh.num_face_vertices[f];

                for (size_t v = 0; v < fv; v++) {
                    tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                    
                    // Position
                    vertices.push_back(info.vertices[3 * idx.vertex_index + 0]);
                    vertices.push_back(info.vertices[3 * idx.vertex_index + 1]);
                    vertices.push_back(info.vertices[3 * idx.vertex_index + 2]);

                    // Normal
                    if (hasNormals) {
                        vertices.push_back(info.normals[3 * idx.normal_index + 0]);
                        vertices.push_back(info.normals[3 * idx.normal_index + 1]);
                        vertices.push_back(info.normals[3 * idx.normal_index + 2]);
                    } else {
                        vertices.push_back(calculatedNormals[idx.vertex_index].x);
                        vertices.push_back(calculatedNormals[idx.vertex_index].y);
                        vertices.push_back(calculatedNormals[idx.vertex_index].z);
                    }
                    
                    // Color
                    int materialId = shape.mesh.material_ids[f];
                    if (materialId < 0 || materials.empty()) {
                        vertices.push_back(0.7f); vertices.push_back(0.7f); vertices.push_back(0.7f);
                    } else {
                        vertices.push_back(materials[materialId].diffuse[0]);
                        vertices.push_back(materials[materialId].diffuse[1]);
                        vertices.push_back(materials[materialId].diffuse[2]);
                        if (!materials[materialId].diffuse_texname.empty() && textureId == 0) {
                            string texturePath = basedir + materials[materialId].diffuse_texname;
                            textureId = loadTexture(texturePath);
                        }
                    }

                    // Texture Coordinate
                    if (idx.texcoord_index >= 0) {
                        vertices.push_back(info.texcoords[2 * idx.texcoord_index + 0]);
                        vertices.push_back(info.texcoords[2 * idx.texcoord_index + 1]);
                    } else {
                        vertices.push_back(0.0f);
                        vertices.push_back(0.0f);
                    }
                }
                indexOffset += fv;
                vertexCount += fv;
            }

            if (!vertices.empty()) {
                Submesh* newShape = new Submesh(vertices.data(), vertices.size() * sizeof(float), vertexCount, textureId);
                loadedObject.shapes.push_back(newShape); 
            }
        }
        return loadedObject;
    }
};
