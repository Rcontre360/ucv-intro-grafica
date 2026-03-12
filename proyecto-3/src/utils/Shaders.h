#pragma once

namespace Shaders {

    const char* vertexShaderSrc = R"glsl(
        #version 330 core

        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec3 aColor;
        layout(location = 3) in vec2 aTexCoords;
        layout(location = 4) in vec3 aTangent;

        out vec3 vPos;
        out vec3 vNormal;
        out vec3 vColor;
        out vec2 vTexCoords;
        out mat3 vTBN;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            vPos = vec3(model * vec4(aPos, 1.0));
            
            mat3 normalMatrix = mat3(transpose(inverse(model)));
            vNormal = normalMatrix * aNormal;
            
            // TBN Matrix calculation for Bump Mapping
            vec3 T = normalize(normalMatrix * aTangent);
            vec3 N = normalize(vNormal);
            // Re-orthogonalize T with respect to N
            T = normalize(T - dot(T, N) * N);
            vec3 B = cross(N, T);
            vTBN = mat3(T, B, N);

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
        in mat3 vTBN;

        out vec4 FragColor;

        struct Light {
            vec3 position;
            vec3 ambient;
            vec3 diffuse;
            vec3 specular;
            bool enabled;
            int shadingMode; 
        };

        #define MAX_LIGHTS 3
        uniform Light lights[MAX_LIGHTS];
        uniform vec3 viewPos;
        uniform bool uEnableFatt;
        
        uniform bool uHasDiffuseMap;
        uniform sampler2D diffuseMap;
        
        uniform bool uHasNormalMap;
        uniform sampler2D normalMap;

        uniform bool uHasAmbientMap;
        uniform sampler2D ambientMap;

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

            vec3 ambient = light.ambient * baseColor;
            vec3 diffuse = (light.diffuse * diff) * baseColor;
            vec3 specular = (light.specular * spec);

            vec3 result = ambient + diffuse + specular;

            if (uEnableFatt) {
                float d = length(light.position - vPos);
                float attenuation = 1.0 / (1.0 + 0.09 * d + 0.032 * (d * d));
                result *= attenuation;
            }

            return result;
        }

        void main() {
            vec3 normal;
            if (uHasNormalMap) {
                normal = texture(normalMap, vTexCoords).rgb;
                normal = normalize(vTBN * (normal * 2.0 - 1.0));
            } else {
                normal = normalize(vNormal);
            }

            if (lights[0].shadingMode == 2) { 
                normal = normalize(cross(dFdx(vPos), dFdy(vPos)));
            }

            vec3 viewDir = normalize(viewPos - vPos);

            // 1. Determine Base Color
            vec3 baseColor = uHasColor ? u_color : vColor;
            if (uHasDiffuseMap) baseColor = texture(diffuseMap, vTexCoords).rgb;

            // 2. Calculate Lighting
            vec3 totalLight = vec3(0.0);
            for (int i = 0; i < MAX_LIGHTS; i++) {
                totalLight += calculateLight(lights[i], normal, viewDir, baseColor);
            }

            // 3. Apply Ambient Occlusion (AO) from map
            if (uHasAmbientMap) {
                float ao = texture(ambientMap, vTexCoords).r;
                totalLight *= ao;
            }

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
        inline static const std::string uHasNormalMap = "uHasNormalMap";
        inline static const std::string normalMap = "normalMap";
        inline static const std::string uHasAmbientMap = "uHasAmbientMap";
        inline static const std::string ambientMap = "ambientMap";
        inline static const std::string uHasColor = "uHasColor";
        inline static const std::string u_color = "u_color";
        inline static const std::string uReflectivity = "uReflectivity";
        inline static const std::string skybox = "skybox";
    };

    const char* lightbulbVertexShaderSrc = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        out vec3 vNormal;
        out vec3 vViewDir;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform vec3 viewPos;
        void main() {
            vec3 worldPos = vec3(model * vec4(aPos, 1.0));
            vNormal = normalize(mat3(transpose(inverse(model))) * aNormal);
            vViewDir = normalize(viewPos - worldPos);
            gl_Position = projection * view * vec4(worldPos, 1.0);
        }
    )glsl";

    const char* lightbulbFragmentShaderSrc = R"glsl(
        #version 330 core
        in vec3 vNormal;
        in vec3 vViewDir;
        out vec4 FragColor;
        uniform vec3 u_color;
        uniform float uIntensity;
        void main() {
            float dotNV = max(dot(vNormal, vViewDir), 0.0);
            vec3 color = u_color * uIntensity;
            vec3 whiteHot = vec3(1.0) * (uIntensity + 1.0);
            float coreFactor = pow(dotNV, 2.0);
            vec3 finalColor = mix(color * 0.5, whiteHot, coreFactor);
            float haloFactor = pow(1.0 - dotNV, 3.0);
            finalColor = mix(finalColor, u_color * uIntensity * 0.2, haloFactor);
            FragColor = vec4(finalColor, 1.0);
        }
    )glsl";

    struct LightbulbShader {
        inline static const std::string model = "model";
        inline static const std::string view = "view";
        inline static const std::string projection = "projection";
        inline static const std::string viewPos = "viewPos";
        inline static const std::string u_color = "u_color";
        inline static const std::string uIntensity = "uIntensity";
    };

    struct BillboardShader {
        inline static const std::string model = "model";
        inline static const std::string view = "view";
        inline static const std::string projection = "projection";
        inline static const std::string u_color = "u_color";
        inline static const std::string uIntensity = "uIntensity";
        inline static const std::string uSize = "uSize";
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
