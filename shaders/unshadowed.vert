#version 320 es

precision highp float;

layout(location = 0) in vec3 in_vPosition_modelspace;
layout(location = 1) in vec2 in_vTextureUV;
layout(location = 2) in vec3 in_vNormal_modelspace;

layout(location = 0) uniform mat4 u_MVP;
layout(location = 1) uniform mat4 u_ModelMatrix;
layout(location = 2) uniform mat3 u_NormalMatrix;

layout(location = 0) out vec2 out_vTextureUV;
layout(location = 1) out vec3 out_vNormal_worldspace;
layout(location = 2) out vec3 out_vPosition_modelspace;
layout(location = 3) out vec3 out_vPosition_worldspace;

void main() {
    gl_Position = u_MVP * vec4(in_vPosition_modelspace, 1.0);
    out_vTextureUV = in_vTextureUV;
    out_vNormal_worldspace = u_NormalMatrix * in_vNormal_modelspace;
    out_vPosition_modelspace = in_vPosition_modelspace;
    out_vPosition_worldspace = (u_ModelMatrix * vec4(in_vPosition_modelspace, 1.0)).xyz;
}
