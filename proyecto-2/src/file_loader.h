#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include "submesh.h"
#include "tinyobjloader.h"
#include "stb/stb_image.h"

using namespace std;

struct LoadedObject {
    std::vector<Submesh*> shapes;
    tinyobj::attrib_t attrib;
};

class FileLoader {
public:
    static GLuint load_texture(const std::string& path) {
        GLuint texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

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

        return texture_id;
    }

    static LoadedObject load_object(const std::string& path) {
        LoadedObject loaded_object;
        tinyobj::attrib_t info;
        vector<tinyobj::shape_t> _shapes;
        vector<tinyobj::material_t> materials;
        string warn, err;
        string basedir = path.substr(0, path.find_last_of("/\\") + 1);

        if (!tinyobj::LoadObj(&info, &_shapes, &materials, &warn, &err, path.c_str(), basedir.c_str())) {
            throw std::runtime_error(warn + err);
        }

        loaded_object.attrib = info;

        for (const auto& shape : _shapes) {
            vector<float> vertices;
            int vertex_count = 0;
            size_t index_offset = 0;
            GLuint texture_id = 0;

            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                int fv = shape.mesh.num_face_vertices[f];

                for (size_t v = 0; v < fv; v++) {
                    tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                    vertices.push_back(info.vertices[3 * idx.vertex_index + 0]);
                    vertices.push_back(info.vertices[3 * idx.vertex_index + 1]);
                    vertices.push_back(info.vertices[3 * idx.vertex_index + 2]);
                    
                    int material_id = shape.mesh.material_ids[f];

                    if (material_id < 0 || materials.empty()) {
                        vertices.push_back(0.7f); vertices.push_back(0.7f); vertices.push_back(0.7f);
                    } else {
                        vertices.push_back(materials[material_id].diffuse[0]);
                        vertices.push_back(materials[material_id].diffuse[1]);
                        vertices.push_back(materials[material_id].diffuse[2]);
                        if (!materials[material_id].diffuse_texname.empty()) {
                            if (texture_id == 0) {
                                string texture_path = basedir + materials[material_id].diffuse_texname;
                                texture_id = load_texture(texture_path);
                            }
                        }
                    }

                    if (idx.texcoord_index >= 0) {
                        vertices.push_back(info.texcoords[2 * idx.texcoord_index + 0]);
                        vertices.push_back(info.texcoords[2 * idx.texcoord_index + 1]);
                    } else {
                        vertices.push_back(0.0f);
                        vertices.push_back(0.0f);
                    }
                }

                index_offset += fv;
                vertex_count += fv;
            }

            if (!vertices.empty()) {
                Submesh* new_shape = new Submesh(vertices.data(), vertices.size() * sizeof(float), vertex_count, texture_id);
                loaded_object.shapes.push_back(new_shape); 
            }
        }
        return loaded_object;
    }
};
