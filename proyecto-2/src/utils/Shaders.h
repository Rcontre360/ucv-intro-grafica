#pragma once

namespace Shaders {

    const char* vertexShaderSrc = R"glsl(
        #version 330 core

        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal; // Normal attribute
        layout(location = 2) in vec3 aColor;  // Color attribute

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

    // --- Normal Visualization Shaders (using Geometry Shader) ---
    const char* normalVertexShaderSrc = R"glsl(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal; // Input normal

        out VS_OUT {
            vec3 normal;
        } vs_out;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main()
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0); // Vertex position in clip space
            // Transform normal to View Space
            mat3 normalMatrix = mat3(transpose(inverse(view * model)));
            vs_out.normal = normalize(normalMatrix * aNormal); // Pass view-space normal to GS
        }
    )glsl";

    const char* normalGeometryShaderSrc = R"glsl(
        #version 330 core
        layout (points) in; // Receive single vertices (points)
        layout (line_strip, max_vertices = 2) out; // Output a line strip of 2 vertices per point

        in VS_OUT {
            vec3 normal;
        } gs_in[];

        uniform float normalLength; // Length of the normal lines

        void main()
        {
            // Emit the start point (the vertex position in clip space)
            gl_Position = gl_in[0].gl_Position; // gl_in[0] since layout (points) in
            EmitVertex();
            
            // Emit the end point, offset by the normal direction in view space
            // gs_in[0].normal is already in view space
            gl_Position = gl_in[0].gl_Position + vec4(gs_in[0].normal, 0.0) * normalLength;
            EmitVertex();
            
            EndPrimitive();
        }
    )glsl";

    const char* normalFragmentShaderSrc = R"glsl(
        #version 330 core
        out vec4 FragColor;

        uniform vec3 u_normal_color; // Color for the normal lines

        void main()
        {
            FragColor = vec4(u_normal_color, 1.0);
        }
    )glsl";

    struct NormalShader {
        inline static const std::string model = "model";
        inline static const std::string view = "view";
        inline static const std::string projection = "projection";
        // normalMatrix is computed in VS, not passed as a uniform to GS/FS
        inline static const std::string normalLength = "normalLength";
        inline static const std::string u_normal_color = "u_normal_color";
    };
    // --- End Normal Visualization Shaders ---

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