#version 320 es

precision highp float;

#define BONE_IS_ROOT 0xFFFFFFFFu

layout(location = 0) in vec3 in_vPosition_modelspace;

layout(location = 3) in uint in_vBone_index_0;
layout(location = 4) in float in_vBone_weight_0;

layout(location = 5) in uint in_vBone_index_1;
layout(location = 6) in float in_vBone_weight_1;

layout(location = 7) in uint in_vBone_index_2;
layout(location = 8) in float in_vBone_weight_2;

layout(location = 9) in uint in_vBone_index_3;
layout(location = 10) in float in_vBone_weight_3;

struct SkeletonGPUElement {
    mat4 offset_matrix;

    uint armature_node_index;
};

layout(std430, binding = 0) buffer SkeletonBuffer {
    SkeletonGPUElement bones[];
} skeleton;

layout(location = 0) uniform mat4 u_LightSpaceMatrix;

void main() {
    gl_Position = u_LightSpaceMatrix * vec4(in_vPosition_modelspace, 1.0);
}
