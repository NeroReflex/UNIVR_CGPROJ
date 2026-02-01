#version 320 es

precision highp float;

layout(location = 0) in vec3 a_Position;

layout(location = 0) uniform mat4 u_LightSpaceMatrix;

void main() {
    gl_Position = u_LightSpaceMatrix * vec4(a_Position, 1.0);
}
