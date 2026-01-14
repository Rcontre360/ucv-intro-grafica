#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <GLFW/glfw3.h>

#include "submesh.h"
#include "tinyobjloader.h"
#include "file_loader.h"

using namespace std;

class State
{
public:
    vector<Submesh*> shapes;
    bool show_vertices = false;
    float vertex_color[3] = { 1.0f, 1.0f, 1.0f };
    float point_size = 5.0f;
    bool show_wireframe = false;
    float wireframe_color[3] = { 1.0f, 1.0f, 1.0f };
    bool show_fill = true;
    bool line_antialiasing = false;
    float global_bounding_box_color[3] = { 0.0f, 1.0f, 0.0f };
    GLuint global_bbox_vao = 0, global_bbox_vbo = 0;

    State(){}

    ~State()
    {
        for (Submesh* obj : shapes) {
            delete obj;
        }
        shapes.clear();
    }


    void delete_submesh(int index)
    {
        if (index >= 0 && index < shapes.size()) {
            delete shapes[index];
            shapes.erase(shapes.begin() + index);
        }
    }

    void draw(GLuint shaderProgram, int selectedSubmeshIndex)
    {
        DrawConfig config;
        config.shaderProgram = shaderProgram;
        config.show_vertices = show_vertices;
        config.vertex_color = vertex_color;
        config.point_size = point_size;
        config.show_wireframe = show_wireframe;
        config.wireframe_color = wireframe_color;
        config.show_fill = show_fill;

        for (size_t i = 0; i < shapes.size(); ++i) {
            config.isSelected = ((int)i == selectedSubmeshIndex);
            shapes[i]->draw(config);
        }
    }

    void load_object(const string& path)
    {
        for (Submesh* obj : shapes) {
            delete obj;
        }
        shapes.clear();

        LoadedObject loaded = FileLoader::load_object(path);
        shapes = loaded.shapes;
        loaded_attrib = loaded.attrib;

        center_shape();
    }

    void rescale_shape(size_t shape_index, float factor)
    {
        if (shape_index < shapes.size()) {
            shapes[shape_index]->scale(glm::vec3(factor));
        }
    }

    void rescaleAllShapes(float factor)
    {
        for (Submesh* obj : shapes) {
            obj->scale(glm::vec3((float)(factor / oldScale)));
        }
        oldScale = factor;
    }

    void translateObject(float deltaX, float deltaY, float deltaZ)
    {
        if (shapes.empty()) return;

        glm::vec3 moveTo = glm::vec3(deltaX, deltaY, deltaZ); 

        for (Submesh* obj : shapes) {
            obj->translate(moveTo); 
        }
    }

    void translateSubmesh(int index, const glm::vec3& offset)
    {
        if (index >= 0 && index < shapes.size()) {
            shapes[index]->translate(offset);
        }
    }

    void rotateObject(float angleX, float angleY)
    {
        if (shapes.empty()) return;

        glm::vec3 min_bound(numeric_limits<float>::max());
        glm::vec3 max_bound(numeric_limits<float>::lowest());
        glm::vec3 pivot = glm::vec3(0.0f, 0.0f, -3.0f); // Assuming this is the object's center after initial setup

        for (Submesh* obj : shapes) {
            obj->rotateAroundPoint(angleX, glm::vec3(1.0f, 0.0f, 0.0f), pivot); // Rotate around X-axis
            obj->rotateAroundPoint(angleY, glm::vec3(0.0f, 1.0f, 0.0f), pivot); // Rotate around Y-axis
        }
    }

private:
    tinyobj::attrib_t loaded_attrib;
    float oldScale = 1.0;

    void center_shape()
    {
        if (shapes.empty() || loaded_attrib.vertices.empty()) {
            return;
        }

        glm::vec3 min_bound(numeric_limits<float>::max());
        glm::vec3 max_bound(numeric_limits<float>::lowest());

        for (size_t i = 0; i < loaded_attrib.vertices.size(); i += 3) {
            min_bound.x = min(min_bound.x, loaded_attrib.vertices[i + 0]);
            min_bound.y = min(min_bound.y, loaded_attrib.vertices[i + 1]);
            min_bound.z = min(min_bound.z, loaded_attrib.vertices[i + 2]);
            max_bound.x = max(max_bound.x, loaded_attrib.vertices[i + 0]);
            max_bound.y = max(max_bound.y, loaded_attrib.vertices[i + 1]);
            max_bound.z = max(max_bound.z, loaded_attrib.vertices[i + 2]);
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
