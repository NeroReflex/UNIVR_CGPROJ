#version 320 es

precision highp float;

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

// Skeleton GPU layout: matches `SkeletonGPUElement` in C++ (offset_matrix + parent + padding)
struct SkeletonGPUElement {
    mat4 offset_matrix;

    uint armature_node_index;
};

layout(std430, binding = 0) buffer SkeletonBuffer {
    SkeletonGPUElement bones[];
} skeleton;

layout(location = 0) uniform mat4 u_MVP;
layout(location = 1) uniform mat4 u_ModelMatrix;
layout(location = 2) uniform mat3 u_NormalMatrix;

layout(location = 0) out vec2 out_vTextureUV;
layout(location = 1) out vec3 out_vNormal_worldspace;
layout(location = 2) out vec3 out_vPosition_modelspace;
layout(location = 3) out vec3 out_vPosition_worldspace;

void main() {
    // Skinning: blend position and normal by up to 4 bones.
    vec4 skinnedPos = vec4(0.0);
    vec3 skinnedNormal = vec3(0.0);

    uint idxs[4] = uint[4](in_vBone_index_0, in_vBone_index_1, in_vBone_index_2, in_vBone_index_3);
    float wts[4] = float[4](in_vBone_weight_0, in_vBone_weight_1, in_vBone_weight_2, in_vBone_weight_3);

    bool anyWeight = false;

    // Safe access: cache the SSBO length and test weight first to avoid
    // out-of-bounds indexing into `skeleton.bones[]` which can stall the GPU.
    uint nbones = uint(skeleton.bones.length());
    for (int i = 0; i < 4; ++i) {
        uint bi = idxs[i];
        float w = wts[i];
        if (w <= 0.0) continue;
        if (bi == BONE_IS_ROOT) continue;
        if (bi >= nbones) continue;
        mat4 bm = skeleton.bones[bi].offset_matrix;
        skinnedPos += bm * vec4(in_vPosition_modelspace, 1.0) * w;
        skinnedNormal += mat3(bm) * in_vNormal_modelspace * w;
        anyWeight = true;
    }

    if (!anyWeight) {
        skinnedPos = vec4(in_vPosition_modelspace, 1.0);
        skinnedNormal = in_vNormal_modelspace;
    }

    gl_Position = u_MVP * skinnedPos;
    out_vTextureUV = in_vTextureUV;
    out_vNormal_worldspace = u_NormalMatrix * skinnedNormal;
    out_vPosition_modelspace = skinnedPos.xyz;
    out_vPosition_worldspace = (u_ModelMatrix * skinnedPos).xyz;
}
