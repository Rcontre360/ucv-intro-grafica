#pragma once

#include <string>
using namespace std;

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

            vec3 T = normalize(normalMatrix * aTangent);
            vec3 N = normalize(vNormal);
            T = normalize(T - dot(T, N) * N); // Gram-Schmidt re-orthogonalization
            vTBN = mat3(T, cross(N, T), N);

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

        uniform bool uHasDiffuseMap;  uniform sampler2D diffuseMap;
        uniform bool uHasNormalMap;   uniform sampler2D normalMap;
        uniform bool uHasAmbientMap;  uniform sampler2D ambientMap;
        uniform bool uHasSpecularMap; uniform sampler2D specularMap;

        uniform bool uHasColor;
        uniform vec3 u_color;
        uniform float uReflectivity;
        uniform samplerCube skybox;
        uniform float uShininess;
        uniform int uSMappingMode;
        uniform int uOMappingMode;
        uniform vec3 uObjCenter;

        vec3 calculateLight(Light light, vec3 normal, vec3 viewDir, vec3 baseColor, float specFactor) {
            if (!light.enabled) return vec3(0.0);

            vec3 n = (light.shadingMode == 2)
                ? normalize(cross(dFdx(vPos), dFdy(vPos)))
                : normal;

            vec3 lightDir = normalize(light.position - vPos);
            float diff = max(dot(n, lightDir), 0.0);

            float spec = 0.0;
            if (light.shadingMode == 1) {
                spec = pow(max(dot(n, normalize(lightDir + viewDir)), 0.0), uShininess);
            } else if (light.shadingMode == 0) {
                spec = pow(max(dot(viewDir, reflect(-lightDir, n)), 0.0), uShininess);
            }

            vec3 result = light.ambient * baseColor
                        + light.diffuse * diff * baseColor
                        + light.specular * spec * specFactor;

            float d = length(light.position - vPos);
            return result / (1.0 + 0.09 * d + 0.032 * d * d);
        }

        vec2 remapTexCoords(vec3 normal) {
            if (uSMappingMode == 0) return vTexCoords;
            vec3 p = normalize((uOMappingMode == 0) ? (vPos - uObjCenter) : normal);
            const float PI = 3.14159265359;
            if (uSMappingMode == 1)
                return vec2(atan(p.z, p.x) / (2.0 * PI) + 0.5, asin(p.y) / PI + 0.5);
            vec3 a = abs(p);
            vec2 uv = (a.x >= a.y && a.x >= a.z) ? vec2(p.z, p.y)
                    : (a.y >= a.x && a.y >= a.z)  ? vec2(p.x, p.z)
                    :                                vec2(p.x, p.y);
            return (uv + 1.0) * 0.5;
        }

        void main() {
            vec3 normal = normalize(vNormal);
            if (uHasNormalMap)
                normal = normalize(vTBN * (texture(normalMap, vTexCoords).rgb * 2.0 - 1.0));

            vec2 tc = remapTexCoords(normal);
            vec3 viewDir = normalize(viewPos - vPos);
            vec3 baseColor = uHasColor ? u_color
                           : (uHasDiffuseMap ? texture(diffuseMap, tc).rgb : vColor);
            float specFactor = uHasSpecularMap ? texture(specularMap, tc).r * 2.0 : 0.1;

            vec3 color = vec3(0.0);
            if (uHasColor) {
                color = baseColor;
            } else {
                for (int i = 0; i < MAX_LIGHTS; i++)
                    color += calculateLight(lights[i], normal, viewDir, baseColor, specFactor);
                if (uHasAmbientMap)
                    color *= texture(ambientMap, tc).r;
                if (uReflectivity > 0.0)
                    color = mix(color, texture(skybox, reflect(-viewDir, normal)).rgb, uReflectivity);
            }

            FragColor = vec4(color, 1.0);
        }
    )glsl";

    struct DefaultShader {
        inline static const string model         = "model";
        inline static const string view          = "view";
        inline static const string projection    = "projection";
        inline static const string viewPos       = "viewPos";
        inline static const string uHasDiffuseMap  = "uHasDiffuseMap";
        inline static const string diffuseMap      = "diffuseMap";
        inline static const string uHasNormalMap   = "uHasNormalMap";
        inline static const string normalMap       = "normalMap";
        inline static const string uHasAmbientMap  = "uHasAmbientMap";
        inline static const string ambientMap      = "ambientMap";
        inline static const string uHasSpecularMap = "uHasSpecularMap";
        inline static const string specularMap     = "specularMap";
        inline static const string uHasColor    = "uHasColor";
        inline static const string u_color      = "u_color";
        inline static const string uReflectivity = "uReflectivity";
        inline static const string uShininess   = "uShininess";
        inline static const string skybox       = "skybox";
        inline static const string uSMappingMode = "uSMappingMode";
        inline static const string uOMappingMode = "uOMappingMode";
        inline static const string uObjCenter   = "uObjCenter";
    };

    const char* skyboxVertexShaderSrc = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        out vec3 TexCoords;
        uniform mat4 projection;
        uniform mat4 view;
        void main() {
            TexCoords = aPos;
            gl_Position = (projection * view * vec4(aPos, 1.0)).xyww;
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
        inline static const string projection = "projection";
        inline static const string view       = "view";
    };
}
