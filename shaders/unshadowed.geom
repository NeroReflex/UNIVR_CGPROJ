#version 320 es

precision highp float;

#define TRIANGLE_VERICES 3

layout (triangles) in;
layout (triangle_strip, max_vertices = TRIANGLE_VERICES) out;

layout(location = 0) in vec2 in_vTextureUV[];
layout(location = 1) in vec3 in_vNormal_worldspace[];
layout(location = 2) in vec3 in_vPosition_modelspace[];
layout(location = 3) in vec3 in_vPosition_worldspace[];

layout(location = 0) uniform mat4 u_MVP;
layout(location = 1) uniform mat4 u_ModelMatrix;
layout(location = 2) uniform mat3 u_NormalMatrix;

layout(location = 0) out vec2 out_vTextureUV;
layout(location = 1) out vec3 out_vNormal_worldspace;
layout(location = 2) out vec3 out_vPosition_worldspace;
layout(location = 3) out flat vec3 out_tangent_worldspace;

void main() {

    vec3 edge1_modelspace = in_vPosition_modelspace[1] - in_vPosition_modelspace[0];
    vec3 edge2_modelspace = in_vPosition_modelspace[2] - in_vPosition_modelspace[0];
    vec2 deltaUV1 = in_vTextureUV[1] - in_vTextureUV[0];
    vec2 deltaUV2 = in_vTextureUV[2] - in_vTextureUV[0];

    // See https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    vec3 tangent = f * ((deltaUV2.y * edge1_modelspace) - deltaUV1.y * (edge2_modelspace));

    // Copy inputs to outputs for each vertex of the triangle
    for (int i = 0; i < TRIANGLE_VERICES; i++) {
        gl_Position = gl_in[i].gl_Position;

        out_vTextureUV = in_vTextureUV[i];
        out_vNormal_worldspace = in_vNormal_worldspace[i];
        out_vPosition_worldspace = in_vPosition_worldspace[i];

        out_tangent_worldspace = normalize( /* u_NormalMatrix * */ tangent);

        // Emit the vertex
        EmitVertex();
    }

    // End the primitive (triangle)
    EndPrimitive();
}
