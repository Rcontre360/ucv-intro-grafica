#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include "tinyobjloader.h"
#include "../objects/Submesh.h"
#include "stb/stb_image.h"

using namespace std;

struct RawTextureData {
    unsigned char* data = nullptr;
    int width = 0;
    int height = 0;
    int channels = 0;

    void free() {
        if (data) stbi_image_free(data);
        data = nullptr;
    }
};

struct SubmeshData {
    vector<Vertex> vertices;
    int materialId;
    RawTextureData diffuseMap;
    RawTextureData specularMap;
    RawTextureData normalMap;
    RawTextureData ambientMap;
};

struct ObjectData {
    string name;
    vector<SubmeshData> submeshes;
    BoundingBox localBox;
};

class FileLoader {
public:
    static RawTextureData loadTextureData(const string& path) {
        RawTextureData raw;
        if (path.empty()) return raw;

        stbi_set_flip_vertically_on_load(true);
        raw.data = stbi_load(path.c_str(), &raw.width, &raw.height, &raw.channels, 0);
        return raw;
    }

    static vector<ObjectData> loadScene(const string& path) {
        vector<ObjectData> result;
        tinyobj::attrib_t info;
        vector<tinyobj::shape_t> shapes;
        vector<tinyobj::material_t> materials;
        string warn, err;
        string basedir = path.substr(0, path.find_last_of("/\\") + 1);

        if (!tinyobj::LoadObj(&info, &shapes, &materials, &warn, &err, path.c_str(), basedir.c_str()))
            throw runtime_error(warn + err);

        for (const auto& shape : shapes) {
            ObjectData objData;
            objData.name = shape.name;

            map<int, vector<Vertex>> materialGroups;
            size_t indexOffset = 0;
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                int materialId = shape.mesh.material_ids[f];
                int fv = shape.mesh.num_face_vertices[f];
                for (size_t v = 0; v < (size_t)fv; v++) {
                    tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                    Vertex vertex;
                    vertex.position = { info.vertices[3 * idx.vertex_index + 0], info.vertices[3 * idx.vertex_index + 1], info.vertices[3 * idx.vertex_index + 2] };

                    if (idx.normal_index >= 0)
                        vertex.normal = { info.normals[3 * idx.normal_index + 0], info.normals[3 * idx.normal_index + 1], info.normals[3 * idx.normal_index + 2] };
                    else vertex.normal = {0.0f, 1.0f, 0.0f};

                    if (!info.texcoords.empty() && idx.texcoord_index >= 0)
                        vertex.texCoords = { info.texcoords[2 * idx.texcoord_index + 0], info.texcoords[2 * idx.texcoord_index + 1] };
                    else vertex.texCoords = {0.0f, 0.0f};

                    if (materialId >= 0 && !materials.empty())
                        vertex.color = { materials[materialId].diffuse[0], materials[materialId].diffuse[1], materials[materialId].diffuse[2] };
                    else vertex.color = {0.7f, 0.7f, 0.7f};

                    materialGroups[materialId].push_back(vertex);
                }
                indexOffset += fv;
            }

            for (auto const& [matId, vertices] : materialGroups) {
                SubmeshData smData;
                smData.vertices = vertices;
                smData.materialId = matId;

                if (matId >= 0 && matId < (int)materials.size()) {
                    const auto& mat = materials[matId];
                    if (!mat.diffuse_texname.empty())  smData.diffuseMap  = loadTextureData(basedir + mat.diffuse_texname);
                    if (!mat.specular_texname.empty()) smData.specularMap = loadTextureData(basedir + mat.specular_texname);
                    if (!mat.bump_texname.empty())     smData.normalMap   = loadTextureData(basedir + mat.bump_texname);
                    if (!mat.ambient_texname.empty())  smData.ambientMap  = loadTextureData(basedir + mat.ambient_texname);
                }

                objData.submeshes.push_back(smData);
            }

            if (!objData.submeshes.empty()) {
                vector<Vertex> allVerts;
                for (const auto& sm : objData.submeshes) allVerts.insert(allVerts.end(), sm.vertices.begin(), sm.vertices.end());
                objData.localBox = getBoundingBox(allVerts);
                result.push_back(objData);
            }
        }
        return result;
    }

    static GLuint uploadTexture(const RawTextureData& raw) {
        if (!raw.data) return 0;
        GLenum format = GL_RGB;
        if (raw.channels == 1) format = GL_RED;
        else if (raw.channels == 4) format = GL_RGBA;

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, raw.width, raw.height, 0, format, GL_UNSIGNED_BYTE, raw.data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return textureID;
    }

    static GLuint loadTexture(const string& path) {
        RawTextureData raw = loadTextureData(path);
        GLuint id = uploadTexture(raw);
        raw.free();
        return id;
    }
};
