#version 320 es

precision highp float;

#define RE_ORTHOGONIZE_TANGENT 1

layout(location = 0) in vec2 in_vTextureUV;
layout(location = 1) in vec3 in_vNormal_worldspace;
layout(location = 2) in vec3 in_vPosition_worldspace;
layout(location = 3) in flat vec3 in_tangent_worldspace;

uniform sampler2D u_DiffuseTex;
uniform sampler2D u_SpecularTex;
uniform sampler2D u_DisplacementTex;

layout(location = 0) uniform mat4 u_MVP;
layout(location = 1) uniform mat4 u_ModelMatrix;
layout(location = 2) uniform mat3 u_NormalMatrix;

layout(location = 3) uniform vec3 u_DiffuseColor;
layout(location = 4) uniform uint u_material_flags;

layout(location = 0) out vec4 gDiffuse;
layout(location = 1) out vec4 gSpecular;
layout(location = 2) out vec4 gNormal;
layout(location = 3) out vec4 gPosition;
layout(location = 4) out vec4 gTangent;

void main() {
    // diffuse texture available: using it
    if ((u_material_flags & 0x1u) != 0u) {
        vec4 tex = texture(u_DiffuseTex, in_vTextureUV);
        if (tex.a < 0.1) { discard; return; }
        // store albedo (diffuse) and specular separately, normals and position
        gDiffuse = vec4(mix(u_DiffuseColor, tex.rgb, tex.a), 0.0);
    } else {
        gDiffuse = vec4(u_DiffuseColor, 0.0);
    }

    if ((u_material_flags & 0x2u) != 0u) {
        gSpecular = vec4(texture(u_SpecularTex, in_vTextureUV).xyz, 0.0);
    } else {
        gSpecular = vec4(0.0);
    }

    vec3 normal_worldspace = normalize(in_vNormal_worldspace);
    if ((u_material_flags & 0x4u) != 0u) {
        vec3 bitangent_worldspace = cross(in_vNormal_worldspace, in_tangent_worldspace);
        mat3 TBN = mat3(in_tangent_worldspace, bitangent_worldspace, in_vNormal_worldspace);
        vec3 normal_map = texture(u_DisplacementTex, in_vTextureUV).xyz;
        normal_map = normal_map * 2.0 - 1.0; // from [0,1] to [-1,1]
        gNormal = vec4(normalize(TBN * normal_map), 0.0);
    } else {
        gNormal = vec4(normal_worldspace, 0.0);
    }

    gPosition = vec4(in_vPosition_worldspace, 1.0);

#if RE_ORTHOGONIZE_TANGENT
    // Gram-Schmidt orthogonalization
    gTangent = vec4(normalize(in_tangent_worldspace - dot(normal_worldspace, in_tangent_worldspace) * normal_worldspace), 0.0);
#else
    gTangent = vec4(in_tangent_worldspace, 0.0);
#endif
}
