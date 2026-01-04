#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <GLFW/glfw3.h>

#include "shape.h"
#include "tinyobjloader.h"

using namespace std;

class State
{
public:
    vector<Submesh*> shapes;

    State(){}

    ~State()
    {
        for (Submesh* obj : shapes) {
            delete obj;
        }
        shapes.clear();
    }

    void update()
    {}

    void draw(GLuint shaderProgram)
    {
        for (Submesh* obj : shapes) {
            obj->draw(shaderProgram);
        }
    }

    void load_object(const string& path)
    {
        tinyobj::attrib_t info;
        vector<tinyobj::shape_t> _shapes;
        vector<tinyobj::material_t> materials;
        string warn, err;
        string basedir = path.substr(0, path.find_last_of("/\\") + 1);

        if (!tinyobj::LoadObj(&info, &_shapes, &materials, &warn, &err, path.c_str(), basedir.c_str())) {
            throw runtime_error(warn + err);
        }

        for (Submesh* obj : shapes) {
            delete obj;
        }

        shapes.clear();
        m_loaded_attrib = info;

        for (const auto& shape : _shapes) {
            vector<float> vertices;
            int vertex_count = 0;
            size_t index_offset = 0;

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
                    }
                }

                index_offset += fv;
                vertex_count += fv;
            }

            if (!vertices.empty()) {
                Submesh* new_shape = new Submesh(vertices.data(), vertices.size() * sizeof(float), vertex_count);
                shapes.push_back(new_shape); 
            }
        }

        center_shape();
    }

    void rescale_shape(size_t shape_index, float factor)
    {
        if (shape_index < shapes.size()) {
            shapes[shape_index]->scale(glm::vec3(factor));
        }
    }

private:
    tinyobj::attrib_t m_loaded_attrib;

    void center_shape()
    {
        if (shapes.empty() || m_loaded_attrib.vertices.empty()) {
            return;
        }

        glm::vec3 min_bound(numeric_limits<float>::max());
        glm::vec3 max_bound(numeric_limits<float>::lowest());

        for (size_t i = 0; i < m_loaded_attrib.vertices.size(); i += 3) {
            min_bound.x = min(min_bound.x, m_loaded_attrib.vertices[i + 0]);
            min_bound.y = min(min_bound.y, m_loaded_attrib.vertices[i + 1]);
            min_bound.z = min(min_bound.z, m_loaded_attrib.vertices[i + 2]);
            max_bound.x = max(max_bound.x, m_loaded_attrib.vertices[i + 0]);
            max_bound.y = max(max_bound.y, m_loaded_attrib.vertices[i + 1]);
            max_bound.z = max(max_bound.z, m_loaded_attrib.vertices[i + 2]);
        }

        glm::vec3 center = (max_bound + min_bound) / 2.0f;
        glm::vec3 size = max_bound - min_bound;
        float max_dim = max({size.x, size.y, size.z});
        float scale_factor = 1.0f / max_dim;

        glm::mat4 to_origin = glm::translate(glm::mat4(1.0f), -center);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor));
        glm::mat4 to_scene = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
        
        glm::mat4 normalization_matrix = to_scene * scale * to_origin;

        for (Submesh* shape : shapes) {
            shape->setTransform(normalization_matrix);
        }
    }
};
