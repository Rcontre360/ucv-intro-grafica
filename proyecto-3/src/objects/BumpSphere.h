#pragma once

#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "Object.h"
#include "Submesh.h"

class BumpSphere : public Object {
public:
    BumpSphere(float radius, unsigned int rings, unsigned int sectors) {
        name = "parametric_bump_sphere";
        vector<Vertex> vertices;

        float const R = 1.0f / (float)(rings - 1);
        float const S = 1.0f / (float)(sectors - 1);

        for (unsigned int r = 0; r < rings; ++r) {
            for (unsigned int s = 0; s < sectors; ++s) {
                float const y = sin(-glm::half_pi<float>() + glm::pi<float>() * r * R);
                float const x = cos(2.0f * glm::pi<float>() * s * S) * sin(glm::pi<float>() * r * R);
                float const z = sin(2.0f * glm::pi<float>() * s * S) * sin(glm::pi<float>() * r * R);

                Vertex v;
                v.position = glm::vec3(x, y, z) * radius;
                v.normal = glm::vec3(x, y, z);
                v.texCoords = glm::vec2(s * S, r * R);
                v.color = glm::vec3(1.0f);

                float u = 2.0f * glm::pi<float>() * s * S;
                v.tangent = glm::normalize(glm::vec3(-sin(u), 0, cos(u)));

                vertices.push_back(v);
            }
        }

        vector<Vertex> triangulated;
        for (unsigned int r = 0; r < rings - 1; ++r) {
            for (unsigned int s = 0; s < sectors - 1; ++s) {
                int i0 = r * sectors + s;
                int i1 = r * sectors + (s + 1);
                int i2 = (r + 1) * sectors + (s + 1);
                int i3 = (r + 1) * sectors + s;

                triangulated.push_back(vertices[i0]);
                triangulated.push_back(vertices[i1]);
                triangulated.push_back(vertices[i2]);

                triangulated.push_back(vertices[i0]);
                triangulated.push_back(vertices[i2]);
                triangulated.push_back(vertices[i3]);
            }
        }

        Submesh* sm = new Submesh(triangulated);
        submeshes.push_back(sm);
        
        glm::vec3 minB(-radius), maxB(radius);
        localBox = {minB, maxB, glm::vec3(0.0f)};
    }
};
