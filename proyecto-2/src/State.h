#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <GLFW/glfw3.h>

#include "Submesh.h"
#include "tinyobjloader.h"
#include "FileLoader.h"
#include "GLUtils.h"

using namespace std;

class State
{
public:
    vector<Submesh*> shapes;
    bool showVertices = false;
    float vertexColor[3] = { 1.0f, 1.0f, 1.0f };
    float pointSize = 5.0f;
    bool showWireframe = false;
    float wireframeColor[3] = { 1.0f, 1.0f, 1.0f };
    bool showFill = true;
    bool lineAntialiasing = false;
    float globalBoundingBoxColor[3] = { 0.0f, 1.0f, 0.0f };
    GLuint globalBboxVao = 0, globalBboxVbo = 0;
    bool showNormals = false;
    float normalLength = 0.1f;
    float normalColor[3] = { 1.0f, 1.0f, 0.0f };

    State(){}

    ~State()
    {
        for (Submesh* obj : shapes) {
            delete obj;
        }
        shapes.clear();
    }


    void deleteSubmesh(int index)
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
        config.showVertices = showVertices;
        config.vertexColor = vertexColor;
        config.pointSize = pointSize;
        config.showWireframe = showWireframe;
        config.wireframeColor = wireframeColor;
        config.showFill = showFill;

        for (size_t i = 0; i < shapes.size(); ++i) {
            config.isSelected = ((int)i == selectedSubmeshIndex);
            shapes[i]->draw(config);
        }
    }

    void drawPicking(GLuint pickingShaderProgram)
    {
        for (size_t i = 0; i < shapes.size(); ++i)
        {
            setGpuVariable(pickingShaderProgram, "objectId", (int)(i + 1));
            shapes[i]->drawForPicking(pickingShaderProgram);
        }
    }

    void drawNormals(GLuint normalShaderProgram)
    {
        setGpuVariable(normalShaderProgram, "normalLength", normalLength);
        setGpuVariable(normalShaderProgram, "normalColor", glm::make_vec3(normalColor));
        for (Submesh* shape : shapes) {
            setGpuVariable(normalShaderProgram, "model", shape->transform);
            shape->drawNormals();
        }
    }

    void loadObject(const string& path)
    {
        for (Submesh* obj : shapes) {
            delete obj;
        }
        shapes.clear();

        LoadedObject loaded = FileLoader::loadObject(path);
        shapes = loaded.shapes;
        loadedAttrib = loaded.attrib;

        centerShape();
    }

    void rescaleShape(size_t shape_index, float factor)
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
    tinyobj::attrib_t loadedAttrib;
    float oldScale = 1.0;

    void centerShape()
    {
        if (shapes.empty() || loadedAttrib.vertices.empty()) {
            return;
        }

        glm::vec3 minBound(numeric_limits<float>::max());
        glm::vec3 maxBound(numeric_limits<float>::lowest());

        for (size_t i = 0; i < loadedAttrib.vertices.size(); i += 3) {
            minBound.x = min(minBound.x, loadedAttrib.vertices[i + 0]);
            minBound.y = min(minBound.y, loadedAttrib.vertices[i + 1]);
            minBound.z = min(minBound.z, loadedAttrib.vertices[i + 2]);
            maxBound.x = max(maxBound.x, loadedAttrib.vertices[i + 0]);
            maxBound.y = max(maxBound.y, loadedAttrib.vertices[i + 1]);
            maxBound.z = max(maxBound.z, loadedAttrib.vertices[i + 2]);
        }

        glm::vec3 center = (maxBound + minBound) / 2.0f;
        glm::vec3 size = maxBound - minBound;
        float maxDim = max({size.x, size.y, size.z});
        float scaleFactor = 1.0f / maxDim;

        glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), -center);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor));
        glm::mat4 toScene = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
        
        glm::mat4 normalizationMatrix = toScene * scale * toOrigin;

        for (Submesh* shape : shapes) {
            shape->setTransform(normalizationMatrix);
        }
    }
};
