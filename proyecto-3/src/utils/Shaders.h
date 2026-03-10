#pragma once

namespace Shaders {

    const char* vertexShaderSrc = R"glsl(
        #version 330 core

        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec3 aColor;
        layout(location = 3) in vec2 aTexCoords;

        out vec3 vPos;
        out vec3 vNormal;
        out vec3 vColor;
        out vec2 vTexCoords;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            vPos = vec3(model * vec4(aPos, 1.0));
            vNormal = mat3(transpose(inverse(model))) * aNormal;
            vColor = aColor;
            vTexCoords = aTexCoords;
            gl_Position = projection * view * vec4(vPos, 1.0);
        }
    )glsl";

    const char* fragmentShaderSrc = R"glsl(
        #version 330 core

        in vec3 vPos;
        in vec3 vNormal;
        in vec3 vColor;
        in vec2 vTexCoords;

        out vec4 FragColor;

        struct Light {
            vec3 position;
            vec3 ambient;
            vec3 diffuse;
            vec3 specular;
            bool enabled;
            int shadingMode; // 0: Phong, 1: Blinn-Phong, 2: Flat
        };

        #define MAX_LIGHTS 3
        uniform Light lights[MAX_LIGHTS];
        uniform vec3 viewPos;
        uniform bool uEnableFatt;
        uniform bool uHasDiffuseMap;
        uniform sampler2D diffuseMap;
        uniform bool uHasColor;
        uniform vec3 u_color;
        uniform float uReflectivity;
        uniform samplerCube skybox;

        vec3 calculateLight(Light light, vec3 normal, vec3 viewDir, vec3 baseColor) {
            if (!light.enabled) return vec3(0.0);

            vec3 lightDir = normalize(light.position - vPos);
            
            // Diffuse
            float diff = max(dot(normal, lightDir), 0.0);
            
            // Specular
            float spec = 0.0;
            if (light.shadingMode == 1) { // Blinn-Phong
                spec = pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), 32.0);
            } else { // Phong
                spec = pow(max(dot(viewDir, reflect(-lightDir, normal)), 0.0), 32.0);
            }

            vec3 result = (light.ambient + light.diffuse * diff) * baseColor + (light.specular * spec);

            if (uEnableFatt) {
                float d = length(light.position - vPos);
                result *= 1.0 / (1.0 + 0.09 * d + 0.032 * (d * d));
            }

            return result;
        }

        void main() {
            // Determine Normal (Automatic flat normal calculation if shading mode is FLAT)
            vec3 normal = (lights[0].shadingMode == 2) ? normalize(cross(dFdx(vPos), dFdy(vPos))) : normalize(vNormal);
            vec3 viewDir = normalize(viewPos - vPos);

            // Determine Base Color
            vec3 baseColor = uHasColor ? u_color : vColor;
            
            // If it's a light orb (uHasColor), make it emissive (ignore standard lighting)
            if (uHasColor) {
                FragColor = vec4(baseColor, 1.0);
                return;
            }

            if (uHasDiffuseMap) baseColor = texture(diffuseMap, vTexCoords).rgb;

            // Calculate Lighting
            vec3 totalLight = vec3(0.0);
            for (int i = 0; i < MAX_LIGHTS; i++) {
                totalLight += calculateLight(lights[i], normal, viewDir, baseColor);
            }

            // Apply Environment Mapping
            if (uReflectivity > 0.0) {
                totalLight = mix(totalLight, texture(skybox, reflect(-viewDir, normal)).rgb, uReflectivity);
            }

            FragColor = vec4(totalLight, 1.0);
        }
    )glsl";

    struct DefaultShader {
        inline static const std::string model = "model";
        inline static const std::string view = "view";
        inline static const std::string projection = "projection";
        inline static const std::string viewPos = "viewPos";
        inline static const std::string uEnableFatt = "uEnableFatt";
        inline static const std::string uHasDiffuseMap = "uHasDiffuseMap";
        inline static const std::string diffuseMap = "diffuseMap";
        inline static const std::string uHasColor = "uHasColor";
        inline static const std::string u_color = "u_color";
        inline static const std::string uReflectivity = "uReflectivity";
        inline static const std::string skybox = "skybox";
    };

    const char* skyboxVertexShaderSrc = R"glsl(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        out vec3 TexCoords;
        uniform mat4 projection;
        uniform mat4 view;
        void main() {
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
        void main() {
            FragColor = texture(skybox, TexCoords);
        }
    )glsl";

    struct SkyboxShader {
        inline static const std::string projection = "projection";
        inline static const std::string view = "view";
    };
}
