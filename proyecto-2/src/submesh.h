#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <algorithm>
#include <limits>

struct DrawConfig {
    GLuint shaderProgram;
    bool isSelected;
    bool show_vertices;
    float* vertex_color;
    float point_size;
    bool show_wireframe;
    float* wireframe_color;
    bool show_fill;
};

class Submesh 
{
public:
    // Transformation matrix
    glm::mat4 transform;

    // OpenGL buffer IDs
    GLuint vao = 0;
    GLuint vbo = 0;
    int vertexCount = 0;
    GLuint texture_id = 0;
    bool has_texture = false;
    float override_color[3] = { 1.0f, 1.0f, 1.0f };
    glm::vec3 min_bound;
    glm::vec3 max_bound;
    bool show_bounding_box = false;
    float bounding_box_color[3] = { 1.0f, 0.0f, 1.0f };
    GLuint bbox_vao = 0, bbox_vbo = 0;

    // Constructor: Initializes transformation and creates OpenGL buffers for the object's geometry.
    Submesh(float* vertices, size_t verticesSize, int count, GLuint tex_id = 0) 
        : transform(glm::mat4(1.0f)), vertexCount(count), texture_id(tex_id)
    {
        has_texture = (texture_id != 0);
        min_bound = glm::vec3(std::numeric_limits<float>::max());
        max_bound = glm::vec3(std::numeric_limits<float>::lowest());
        for (int i = 0; i < count; ++i) {
            float x = vertices[i * 11 + 0];
            float y = vertices[i * 11 + 1];
            float z = vertices[i * 11 + 2];
            min_bound.x = std::min(min_bound.x, x);
            min_bound.y = std::min(min_bound.y, y);
            min_bound.z = std::min(min_bound.z, z);
            max_bound.x = std::max(max_bound.x, x);
            max_bound.y = std::max(max_bound.y, y);
            max_bound.z = std::max(max_bound.z, z);
        }

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, verticesSize, vertices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Color attribute
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // Texture coordinate attribute
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);

        setup_bounding_box();
    }

    // Destructor: Cleans up OpenGL buffers.
    ~Submesh()
    {
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        if (bbox_vbo) glDeleteBuffers(1, &bbox_vbo);
        if (bbox_vao) glDeleteVertexArrays(1, &bbox_vao);
        if (has_texture && texture_id) glDeleteTextures(1, &texture_id);
    }

    void draw_for_picking(GLuint shaderProgram)
    {
        GLint model = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(transform));
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    // Draws the submesh using the provided shader program.
    void draw(const DrawConfig& config)
    {
        GLint model = glGetUniformLocation(config.shaderProgram, "model");
        glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(transform));

        GLint isSelectedLoc = glGetUniformLocation(config.shaderProgram, "isSelected");
        glUniform1i(isSelectedLoc, config.isSelected);

        if (config.isSelected) {
            GLint overrideColorLoc = glGetUniformLocation(config.shaderProgram, "u_override_color");
            glUniform3fv(overrideColorLoc, 1, override_color);
        }

        GLint hasTextureLoc = glGetUniformLocation(config.shaderProgram, "uHasTexture");
        glUniform1i(hasTextureLoc, has_texture);

        if (has_texture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            glUniform1i(glGetUniformLocation(config.shaderProgram, "uTexture"), 0);
        }

        GLint renderPointsLoc = glGetUniformLocation(config.shaderProgram, "u_render_points");
        glUniform1i(renderPointsLoc, 0);
        
        GLint isWireframeLoc = glGetUniformLocation(config.shaderProgram, "u_is_wireframe");
        glUniform1i(isWireframeLoc, 0);

        glBindVertexArray(vao);
        if (config.show_fill) {
            glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        }

        if (config.show_wireframe && config.wireframe_color) {
            glUniform1i(isWireframeLoc, 1);
            GLint wColorLoc = glGetUniformLocation(config.shaderProgram, "u_wireframe_color");
            glUniform3fv(wColorLoc, 1, config.wireframe_color);
            
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawArrays(GL_TRIANGLES, 0, vertexCount);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glUniform1i(isWireframeLoc, 0);
        }

        if (config.isSelected && show_bounding_box) {
            glUniform1i(isWireframeLoc, 1);
            GLint wColorLoc = glGetUniformLocation(config.shaderProgram, "u_wireframe_color");
            glUniform3fv(wColorLoc, 1, bounding_box_color);

            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1.0, -1.0);

            glBindVertexArray(bbox_vao);
            glDrawArrays(GL_LINES, 0, 24);
            glBindVertexArray(0);

            glDisable(GL_POLYGON_OFFSET_LINE);
            glUniform1i(isWireframeLoc, 0);
        }

        if (config.show_vertices && config.vertex_color) {
            glUniform1i(renderPointsLoc, 1);
            GLint vColorLoc = glGetUniformLocation(config.shaderProgram, "vertexColor");
            glUniform3fv(vColorLoc, 1, config.vertex_color);
            GLint pointSizeLoc = glGetUniformLocation(config.shaderProgram, "u_point_size");
            glUniform1f(pointSizeLoc, config.point_size);
            glDrawArrays(GL_POINTS, 0, vertexCount);
        }
    }

    void translate(const glm::vec3& offset) { 
        transform = glm::translate(transform, offset); 
    }

    void rotate(float angle, const glm::vec3& axis) { 
        transform = glm::rotate(transform, glm::radians(angle), axis); 
    }

    void scale(const glm::vec3& factor) { 
        transform = glm::scale(transform, factor); 
    }

    const glm::mat4& getTransform() const { 
        return transform; 
    }

    void setTransform(const glm::mat4& newTransform) { 
        transform = newTransform; 
    }

    void resetTransform() { 
        transform = glm::mat4(1.0f); 
    }

    void rotateAroundPoint(float angle, const glm::vec3& axis, const glm::vec3& pivot) {
        transform = glm::translate(glm::mat4(1.0f), pivot) * 
                    glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis) * 
                    glm::translate(glm::mat4(1.0f), -pivot) * 
                    transform;
    }

private:
    void setup_bounding_box() {
        float vertices[] = {
            min_bound.x, min_bound.y, min_bound.z,
            max_bound.x, min_bound.y, min_bound.z,
            max_bound.x, max_bound.y, min_bound.z,
            min_bound.x, max_bound.y, min_bound.z,
            min_bound.x, min_bound.y, max_bound.z,
            max_bound.x, min_bound.y, max_bound.z,
            max_bound.x, max_bound.y, max_bound.z,
            min_bound.x, max_bound.y, max_bound.z
        };

        unsigned int indices[] = {
            0, 1, 1, 2, 2, 3, 3, 0,
            4, 5, 5, 6, 6, 7, 7, 4,
            0, 4, 1, 5, 2, 6, 3, 7
        };
        
        float line_vertices[24 * 3];
        for(int i = 0; i < 24; ++i) {
            line_vertices[i*3+0] = vertices[indices[i]*3+0];
            line_vertices[i*3+1] = vertices[indices[i]*3+1];
            line_vertices[i*3+2] = vertices[indices[i]*3+2];
        }

        glGenVertexArrays(1, &bbox_vao);
        glGenBuffers(1, &bbox_vbo);

        glBindVertexArray(bbox_vao);
        glBindBuffer(GL_ARRAY_BUFFER, bbox_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};