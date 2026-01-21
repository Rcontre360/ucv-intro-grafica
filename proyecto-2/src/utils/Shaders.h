#pragma once

namespace Shaders {

    const char* vertexShaderSrc = R"glsl(
        #version 330 core

        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;

        out vec3 vColor;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform float u_point_size;

        void main() 
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            vColor = aColor;
            gl_PointSize = u_point_size;
        }
    )glsl";

    const char* fragmentShaderSrc = R"glsl(
        #version 330 core

        in vec3 vColor;

        out vec4 FragColor;

        uniform bool uHasColor;
        uniform sampler2D uTexture;
        uniform vec3 u_color;

        void main() {
            FragColor = vec4(vColor, 1.0);

            if (uHasColor){
                FragColor = vec4(u_color, 1.0);
            }
        }
    )glsl";

    struct DefaultShader {
        inline static const std::string model = "model";
        inline static const std::string view = "view";
        inline static const std::string projection = "projection";
        inline static const std::string u_point_size = "u_point_size";
        inline static const std::string uHasColor = "uHasColor";
        inline static const std::string u_color = "u_color";
    };

    const char* pickingVertexShaderSrc = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main()
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )glsl";

    const char* pickingFragmentShaderSrc = R"glsl(
        #version 330 core
        out vec4 FragColor;
        uniform int objectId;
        void main()
        {
            FragColor = vec4(
                float((objectId >> 0) & 0xFF) / 255.0f,
                float((objectId >> 8) & 0xFF) / 255.0f,
                float((objectId >> 16) & 0xFF) / 255.0f,
                1.0f
            );
        }
    )glsl";

    struct PickingShader {
        inline static const std::string model = "model";
        inline static const std::string view = "view";
        inline static const std::string projection = "projection";
        inline static const std::string objectId = "objectId";
    };
}