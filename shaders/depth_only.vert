#version 320 es

precision highp float;

#define BONE_IS_ROOT 0xFFFFFFFFu

#define BONE_IS_ROOT 0xFFFFFFFFu

#define MAX_BONES 128u

layout(location = 0) in vec3 in_vPosition_modelspace;
layout(location = 1) in vec2 in_vTextureUV;
layout(location = 2) in vec3 in_vNormal_modelspace;

layout(location = 3) in uint in_vBone_index_0;
layout(location = 4) in float in_vBone_weight_0;

layout(location = 5) in uint in_vBone_index_1;
layout(location = 6) in float in_vBone_weight_1;

layout(location = 7) in uint in_vBone_index_2;
layout(location = 8) in float in_vBone_weight_2;

layout(location = 9) in uint in_vBone_index_3;
layout(location = 10) in float in_vBone_weight_3;

layout(std430, binding = 0) buffer SkeletonBuffer {
    mat4 offset_matrix[];
} skeleton;

layout(location = 0) uniform mat4 u_LightSpaceMatrix;

void main() {
    vec4 skinnedPos = vec4(0.0);
    vec3 skinnedNormal = vec3(0.0);

    uint idxs[4] = uint[4](in_vBone_index_0, in_vBone_index_1, in_vBone_index_2, in_vBone_index_3);
    float wts[4] = float[4](in_vBone_weight_0, in_vBone_weight_1, in_vBone_weight_2, in_vBone_weight_3);

    bool anyWeight = false;

    for (int i = 0; i < 4; ++i) {
        uint bi = idxs[i];
        float w = wts[i];
        if (w <= 0.0) continue;
        if (bi == BONE_IS_ROOT) continue;
        mat4 bm = skeleton.offset_matrix[bi];
        skinnedPos += bm * vec4(in_vPosition_modelspace, 1.0) * w;
        skinnedNormal += mat3(bm) * in_vNormal_modelspace * w;
        anyWeight = true;
    }

    if (!anyWeight) {
        skinnedPos = vec4(in_vPosition_modelspace, 1.0);
        skinnedNormal = in_vNormal_modelspace;
    }

    gl_Position = u_LightSpaceMatrix * skinnedPos;
}
