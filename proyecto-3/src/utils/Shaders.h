#pragma once

namespace Shaders {

    // we keep normal bc we will use on proy3
    const char* vertexShaderSrc = R"glsl(
        #version 330 core

        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec3 aColor; 
        layout(location = 3) in vec2 aTexCoords;

        out vec3 vColor;
        out vec2 vTexCoords;
        out vec3 vNormal;
        out vec3 vFragPos;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() 
        {
            vFragPos = vec3(model * vec4(aPos, 1.0));
            vNormal = mat3(transpose(inverse(model))) * aNormal;
            vColor = aColor;
            vTexCoords = aTexCoords;
            gl_Position = projection * view * vec4(vFragPos, 1.0);
        }
    )glsl";

    const char* fragmentShaderSrc = R"glsl(
        #version 330 core

        in vec3 vColor;
        in vec2 vTexCoords;
        in vec3 vNormal;
        in vec3 vFragPos;

        out vec4 FragColor;

        uniform bool uHasDiffuseMap;
        uniform sampler2D diffuseMap;
        uniform vec3 u_color;
        uniform bool uHasColor;

        void main() {
            vec3 color = vColor;
            if (uHasDiffuseMap) {
                color = texture(diffuseMap, vTexCoords).rgb;
            }
            
            if (uHasColor) {
                color = u_color;
            }

            FragColor = vec4(color, 1.0);
        }
    )glsl";

    struct DefaultShader {
        inline static const std::string model = "model";
        inline static const std::string view = "view";
        inline static const std::string projection = "projection";
        inline static const std::string u_point_size = "u_point_size";
        inline static const std::string uHasColor = "uHasColor";
        inline static const std::string u_color = "u_color";
        inline static const std::string uHasDiffuseMap = "uHasDiffuseMap";
        inline static const std::string diffuseMap = "diffuseMap";
    };

    const char* skyboxVertexShaderSrc = R"glsl(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        out vec3 TexCoords;

        uniform mat4 projection;
        uniform mat4 view;

        void main()
        {
            TexCoords = aPos;
            vec4 pos = projection * view * vec4(aPos, 1.0);
            gl_Position = pos.xyww;
        }
    )glsl";

    const char* skyboxFragmentShaderSrc = R"glsl(
        #version 330 core
        out vec4 FragColor;

        in vec3 TexCoords;

        uniform samplerCube skybox;

        void main()
        {    
            FragColor = texture(skybox, TexCoords);
        }
    )glsl";

    struct SkyboxShader {
        inline static const std::string projection = "projection";
        inline static const std::string view = "view";
        inline static const std::string skybox = "skybox";
    };
}
