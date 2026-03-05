#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <algorithm>
#include <limits>

#include "Base.h"
#include "../utils/Utils.h"
#include "../utils/Camera.h" // Added for Camera::getInstance()

using namespace std;

class Submesh : public BaseSubmesh 
{
public:
    Submesh(const vector<Vertex>& vertices) : BaseSubmesh(vertices){
    }

    ~Submesh()
    {
    }

    void draw(const DrawConfig& config){
        BaseSubmesh::draw(config);
    }
};
