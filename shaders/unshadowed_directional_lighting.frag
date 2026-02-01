#version 320 es

precision highp float;

layout(location = 0) in vec2 v_TexCoord;

uniform sampler2D u_GDiffuse;
uniform sampler2D u_GSpecular;
uniform sampler2D u_GNormal;
uniform sampler2D u_GPosition;

uniform sampler2D u_LightpassInput;

struct DirectionalLight { vec4 dir; vec4 color; };

layout(location = 0) uniform vec3 u_LightDir;
layout(location = 1) uniform vec3 u_LightColor;

layout(location = 0) out vec3 o_LightpassOutput;

void main() {
    vec3 n = normalize(texture(u_GNormal, v_TexCoord).rgb);
    vec3 albedo = texture(u_GDiffuse, v_TexCoord).rgb;

    // position available if needed: vec3 pos = texture(u_GPosition, v_TexCoord).xyz;

    // load the input lightpass color: this is the light accumulated so far
    vec3 result = texture(u_LightpassInput, v_TexCoord).rgb;

    // accumulate the contribution of the current directional light
    vec3 ldir = normalize(u_LightDir);
    float NdotL = max(dot(n, -ldir), 0.0);
    result += u_LightColor * albedo * NdotL;

    o_LightpassOutput = result;
}
