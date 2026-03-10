#pragma once

namespace Shaders {

    const char* vertexShaderSrc = R"glsl(
        #version 330 core

        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec3 aColor; 
        layout(location = 3) in vec2 aTexCoords;

        out vec3 vColor;
        out vec2 vTexCoords;
        flat out vec3 vNormalFlat;
        out vec3 vNormal;
        out vec3 vFragPos;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() 
        {
            vFragPos = vec3(model * vec4(aPos, 1.0));
            vNormal = mat3(transpose(inverse(model))) * aNormal;
            vNormalFlat = vNormal;
            vColor = aColor;
            vTexCoords = aTexCoords;
            gl_Position = projection * view * vec4(vFragPos, 1.0);
        }
    )glsl";

    const char* fragmentShaderSrc = R"glsl(
        #version 330 core

        struct Light {
            vec3 position;
            vec3 ambient;
            vec3 diffuse;
            vec3 specular;
            bool enabled;
            int shadingMode; // 0: Phong, 1: Blinn-Phong, 2: Flat
        };

        in vec3 vColor;
        in vec2 vTexCoords;
        flat in vec3 vNormalFlat;
        in vec3 vNormal;
        in vec3 vFragPos;

        out vec4 FragColor;

        uniform bool uHasDiffuseMap;
        uniform sampler2D diffuseMap;
        uniform vec3 u_color;
        uniform bool uHasColor;
        
        uniform vec3 viewPos;
        uniform Light lights[3];
        uniform bool uEnableFatt;

        vec3 calculateLight(Light light, vec3 normal, vec3 viewDir) {
            if (!light.enabled) return vec3(0.0);

            // Ambient
            vec3 ambient = light.ambient * (uHasDiffuseMap ? texture(diffuseMap, vTexCoords).rgb : vColor);

            // Diffuse
            vec3 lightDir = normalize(light.position - vFragPos);
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = light.diffuse * diff * (uHasDiffuseMap ? texture(diffuseMap, vTexCoords).rgb : vColor);

            // Specular
            float spec = 0.0;
            if (light.shadingMode == 1) { // Blinn-Phong
                vec3 halfwayDir = normalize(lightDir + viewDir);
                spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
            } else { // Phong or Flat
                vec3 reflectDir = reflect(-lightDir, normal);
                spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
            }
            vec3 specular = light.specular * spec;

            // Attenuation (fatt)
            float distance = length(light.position - vFragPos);
            float fatt = 1.0;
            if (uEnableFatt) {
                // Modified constants for lower focal intensity and higher spread
                fatt = 1.0 / (1.5 + 0.022 * distance + 0.0019 * (distance * distance));
            }

            return (ambient + diffuse + specular) * fatt;
        }

        void main() {
            vec3 viewDir = normalize(viewPos - vFragPos);
            vec3 totalLight = vec3(0.0);

            for(int i = 0; i < 3; i++) {
                vec3 normal = (lights[i].shadingMode == 2) ? normalize(vNormalFlat) : normalize(vNormal);
                totalLight += calculateLight(lights[i], normal, viewDir);
            }

            if (uHasColor) {
                FragColor = vec4(u_color, 1.0);
            } else {
                FragColor = vec4(totalLight, 1.0);
            }
        }
    )glsl";

    struct DefaultShader {
        inline static const std::string model = "model";
        inline static const std::string view = "view";
        inline static const std::string projection = "projection";
        inline static const std::string uHasColor = "uHasColor";
        inline static const std::string u_color = "u_color";
        inline static const std::string uHasDiffuseMap = "uHasDiffuseMap";
        inline static const std::string diffuseMap = "diffuseMap";
        inline static const std::string viewPos = "viewPos";
        inline static const std::string uEnableFatt = "uEnableFatt";
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