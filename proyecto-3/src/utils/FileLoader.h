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

struct SubmeshData {
    vector<Vertex> vertices;
    int materialId;
    GLuint diffuseMap = 0;
    GLuint specularMap = 0;
    GLuint normalMap = 0;
    GLuint ambientMap = 0;
};

// Represents a single Blender object (o tag) which might have multiple materials (submeshes)
struct ObjectData {
    string name;
    vector<SubmeshData> submeshes;
    BoundingBox localBox;
};

struct LoadedScene {
    vector<ObjectData> objects;
    vector<tinyobj::material_t> materials;
    BoundingBox sceneBox;
};

class FileLoader {
private:
    static GLuint loadTexture(const string& path) {
        if (path.empty()) return 0;
        
        stbi_set_flip_vertically_on_load(true);
        int width, height, nrComponents;
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrComponents == 1) format = GL_RED;
            else if (nrComponents == 3) format = GL_RGB;
            else if (nrComponents == 4) format = GL_RGBA;

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
        }
        return 0;
    }

public:
    static LoadedScene loadScene(const std::string& path) {
        LoadedScene loadedScene;
        tinyobj::attrib_t info;
        vector<tinyobj::shape_t> shapes;
        string warn, err;
        string basedir = path.substr(0, path.find_last_of("/\\") + 1);

        if (!tinyobj::LoadObj(&info, &shapes, &loadedScene.materials, &warn, &err, path.c_str(), basedir.c_str())) {
            throw std::runtime_error(warn + err);
        }

        glm::vec3 sceneMin(numeric_limits<float>::max());
        glm::vec3 sceneMax(numeric_limits<float>::lowest());

        for (const auto& shape : shapes) {
            ObjectData objData;
            objData.name = shape.name;
            glm::vec3 objMin(numeric_limits<float>::max());
            glm::vec3 objMax(numeric_limits<float>::lowest());

            map<int, vector<Vertex>> materialGroups;
            size_t indexOffset = 0;
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                int materialId = shape.mesh.material_ids[f];
                int fv = shape.mesh.num_face_vertices[f];
                for (size_t v = 0; v < (size_t)fv; v++) {
                    tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                    Vertex vertex;
                    vertex.position = { info.vertices[3 * idx.vertex_index + 0], info.vertices[3 * idx.vertex_index + 1], info.vertices[3 * idx.vertex_index + 2] };
                    
                    // Update bounding boxes
                    objMin = glm::min(objMin, vertex.position);
                    objMax = glm::max(objMax, vertex.position);
                    sceneMin = glm::min(sceneMin, vertex.position);
                    sceneMax = glm::max(sceneMax, vertex.position);

                    if (idx.normal_index >= 0) vertex.normal = { info.normals[3 * idx.normal_index + 0], info.normals[3 * idx.normal_index + 1], info.normals[3 * idx.normal_index + 2] };
                    else vertex.normal = {0.0f, 1.0f, 0.0f};
                    
                    if (!info.texcoords.empty() && idx.texcoord_index >= 0) vertex.texCoords = { info.texcoords[2 * idx.texcoord_index + 0], info.texcoords[2 * idx.texcoord_index + 1] };
                    else vertex.texCoords = {0.0f, 0.0f};
                    
                    if (materialId >= 0 && !loadedScene.materials.empty()) vertex.color = { loadedScene.materials[materialId].diffuse[0], loadedScene.materials[materialId].diffuse[1], loadedScene.materials[materialId].diffuse[2] };
                    else vertex.color = {0.7f, 0.7f, 0.7f};
                    
                    materialGroups[materialId].push_back(vertex);
                }
                indexOffset += fv;
            }

            for (auto const& [matId, vertices] : materialGroups) {
                SubmeshData smData;
                smData.vertices = vertices;
                smData.materialId = matId;

                if (matId >= 0 && matId < (int)loadedScene.materials.size()) {
                    const auto& mat = loadedScene.materials[matId];
                    if (!mat.diffuse_texname.empty()) smData.diffuseMap = loadTexture(basedir + mat.diffuse_texname);
                    if (!mat.specular_texname.empty()) smData.specularMap = loadTexture(basedir + mat.specular_texname);
                    if (!mat.bump_texname.empty()) smData.normalMap = loadTexture(basedir + mat.bump_texname);
                    if (!mat.ambient_texname.empty()) smData.ambientMap = loadTexture(basedir + mat.ambient_texname);
                }

                objData.submeshes.push_back(smData);
            }

            if (!objData.submeshes.empty()) {
                objData.localBox = { objMin, objMax, (objMin + objMax) * 0.5f };
                loadedScene.objects.push_back(objData);
            }
        }
        loadedScene.sceneBox = { sceneMin, sceneMax, (sceneMin + sceneMax) * 0.5f };
        return loadedScene;
    }
};
