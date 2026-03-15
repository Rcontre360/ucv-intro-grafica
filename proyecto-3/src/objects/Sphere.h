#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "Object.h"
#include "Submesh.h"

class Sphere : public Object {
public:
    Sphere(float radius, glm::vec3 position = glm::vec3(0.0f)) {
        name = "sphere";
        int segs = max(8, (int)(radius * 80.0f));

        vector<Vertex> grid;
        for (int r = 0; r <= segs; r++) {
            for (int s = 0; s <= segs; s++) {
                float u = (float)s / segs;
                float v = (float)r / segs;
                float x = cos(2*glm::pi<float>()*u) * sin(glm::pi<float>()*v);
                float y = cos(glm::pi<float>()*v);
                float z = sin(2*glm::pi<float>()*u) * sin(glm::pi<float>()*v);
                Vertex vert;
                vert.position = glm::vec3(x, y, z) * radius;
                vert.normal   = glm::vec3(x, y, z);
                vert.texCoords = glm::vec2(u, v);
                vert.color    = glm::vec3(1.0f);
                vert.tangent  = glm::normalize(glm::vec3(-sin(2*glm::pi<float>()*u), 0, cos(2*glm::pi<float>()*u)));
                grid.push_back(vert);
            }
        }

        vector<Vertex> tris;
        for (int r = 0; r < segs; r++) {
            for (int s = 0; s < segs; s++) {
                int i0 = r*(segs+1)+s, i1=i0+1, i2=(r+1)*(segs+1)+s+1, i3=(r+1)*(segs+1)+s;
                tris.push_back(grid[i0]); tris.push_back(grid[i1]); tris.push_back(grid[i2]);
                tris.push_back(grid[i0]); tris.push_back(grid[i2]); tris.push_back(grid[i3]);
            }
        }

        Submesh* sm = new Submesh(tris);
        sm->setTranslate(position);
        sm->initialTransform = sm->getTransform();
        submeshes.push_back(sm);

        center   = position;
        localBox = ::getBoundingBox(tris);
    }
};
