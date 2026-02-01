#version 320 es

precision highp float;

#define ENABLE_SSAO 1

layout(location = 0) in vec2 v_TexCoord;

uniform sampler2D u_GDiffuse;

uniform sampler2D u_SSAO;

layout(location = 0) uniform vec3 u_Ambient;

layout(location = 0) out vec3 o_LightpassOutput;

void main() {
    vec3 albedo = texture(u_GDiffuse, v_TexCoord).rgb;

#if defined(ENABLE_SSAO)
    float ssao_coeff = texture(u_SSAO, v_TexCoord).r;
#else
    const float ssao_coeff = 1.0;
#endif

    o_LightpassOutput = u_Ambient * albedo * ssao_coeff;
}
