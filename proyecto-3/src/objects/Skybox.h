#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include "stb/stb_image.h"
#include "../utils/Shaders.h"
#include "../utils/Camera.h"
#include "../utils/Utils.h"

class Skybox {
public:
    unsigned int textureID;
    unsigned int vao, vbo;

    Skybox(const string& path) {
        setupMesh();
        textureID = loadCubemapFromGrid(path);
    }

    ~Skybox() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteTextures(1, &textureID);
    }

    void draw(GLuint shaderProgram) {
        glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content
        glUseProgram(shaderProgram);
        
        glm::mat4 view = glm::mat4(glm::mat3(Camera::getInstance().getViewMatrix())); // remove translation from the view matrix
        glm::mat4 projection = Camera::getInstance().projection;
        
        setGpuVariable(shaderProgram, Shaders::SkyboxShader::view, view);
        setGpuVariable(shaderProgram, Shaders::SkyboxShader::projection, projection);

        // skybox cube
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default
    }

private:
    void setupMesh() {
        float skyboxVertices[] = {
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }

    unsigned int loadCubemapFromGrid(const string& path) {
        unsigned int texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

        int width, height, nrChannels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            int faceWidth = width / 4;
            int faceHeight = height / 3;

            // Define coordinates for 4x3 grid
            // Grid layout (typical):
            // [   ] [Top ] [   ] [   ]
            // [Left] [Front] [Right] [Back]
            // [   ] [Bottom] [   ] [   ]
            
            struct FacePos { int col, row; GLenum target; };
            vector<FacePos> faces = {
                {2, 1, GL_TEXTURE_CUBE_MAP_POSITIVE_X}, // Right
                {0, 1, GL_TEXTURE_CUBE_MAP_NEGATIVE_X}, // Left
                {1, 0, GL_TEXTURE_CUBE_MAP_POSITIVE_Y}, // Top
                {1, 2, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y}, // Bottom
                {1, 1, GL_TEXTURE_CUBE_MAP_POSITIVE_Z}, // Front
                {3, 1, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z}  // Back
            };

            for (const auto& face : faces) {
                unsigned char* faceData = new unsigned char[faceWidth * faceHeight * nrChannels];
                for (int y = 0; y < faceHeight; ++y) {
                    int srcY = face.row * faceHeight + y;
                    memcpy(faceData + y * faceWidth * nrChannels, 
                           data + (srcY * width + face.col * faceWidth) * nrChannels, 
                           faceWidth * nrChannels);
                }
                glTexImage2D(face.target, 0, (nrChannels == 4 ? GL_RGBA : GL_RGB), faceWidth, faceHeight, 0, (nrChannels == 4 ? GL_RGBA : GL_RGB), GL_UNSIGNED_BYTE, faceData);
                delete[] faceData;
            }

            stbi_image_free(data);

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        } else {
            cerr << "Cubemap grid failed to load at path: " << path << endl;
            stbi_image_free(data);
        }

        return texID;
    }
};
