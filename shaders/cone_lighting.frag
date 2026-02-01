#version 320 es

precision highp float;

#define LIGHTING_IN_TANGENTSPACE 0

#define SMOOTH_BIAS 1

layout(location = 0) in vec2 v_TexCoord;

uniform sampler2D u_GDiffuse;
uniform sampler2D u_GSpecular;
uniform sampler2D u_GNormalTangentSpace;
uniform sampler2D u_GPosition;
uniform sampler2D u_GTangent;
uniform sampler2D u_GNormal;

uniform sampler2D u_LightpassInput;

uniform sampler2D u_LDepthTexture;

layout(location = 0) uniform mat4 u_LightSpaceMatrix;

layout(location = 1) uniform vec3 u_LightColor;
layout(location = 2) uniform vec3 u_LightPosition;
layout(location = 3) uniform vec3 u_LightDirection;


layout(location = 0) out vec3 o_LightpassOutput;

void main() {
    vec3 vNormal_worldspace = texture(u_GNormal, v_TexCoord).rgb;
    vec3 vDiffuse = texture(u_GDiffuse, v_TexCoord).rgb;
    vec4 vPosition_worldspace = vec4(texture(u_GPosition, v_TexCoord).xyz, 1.0);

    ivec2 shadowTextureDimensions = textureSize(u_LDepthTexture, 0);

    // load the input lightpass color: this is the light accumulated so far
    vec3 result = texture(u_LightpassInput, v_TexCoord).rgb;

    vec4 lightSpacePos = u_LightSpaceMatrix * vPosition_worldspace;
    lightSpacePos /= lightSpacePos.w;
    vec2 shadowTexCoord = lightSpacePos.xy * 0.5 + 0.5;
    if (shadowTexCoord.x < 0.0 || shadowTexCoord.x > 1.0 || shadowTexCoord.y < 0.0 || shadowTexCoord.y > 1.0) {
        o_LightpassOutput = result;
        return;
    }

    float distanceFromCenter = length(shadowTexCoord - vec2(0.5));
    if (distanceFromCenter > 0.5) {
        o_LightpassOutput = result;
        return;
    }

#if SMOOTH_BIAS
    float bias = max(0.01 * distanceFromCenter, 0.0001);
#else
    const float bias = 0.0000001;
#endif

    //vec2 texelSize = 1.0 / vec2(float(shadowTextureDimensions.x), float(shadowTextureDimensions.y));
    vec2 texelPos = shadowTexCoord * vec2(float(shadowTextureDimensions.x), float(shadowTextureDimensions.y));

    float closestDepth = texelFetch(u_LDepthTexture, ivec2(texelPos), 0).r;

    float currentDepth = lightSpacePos.z * 0.5 + 0.5;

    vec3 vDir = normalize(vPosition_worldspace.xyz - u_LightPosition);

#if LIGHTING_IN_TANGENTSPACE
    vec3 tangent_worldspace = texture(u_GTangent, v_TexCoord).xyz;
    vec3 bitangent_worldspace = cross(vNormal_worldspace, tangent_worldspace);
    mat3 TBN = mat3(tangent_worldspace, bitangent_worldspace, vNormal_worldspace);

    mat3 invTBN = transpose(TBN);

    //vec3 vLightDir_worldspace = normalize(u_LightPosition - vPosition_worldspace.xyz);
    vec3 vLightDir_tangentspace = normalize(invTBN * u_LightDirection);

    // vDir has to be in tangent space too
    vDir = normalize(invTBN * vDir);

    float fCosine = max(dot(vLightDir_tangentspace, vDir), 0.0);
#else
    // vDir is already in worldspace
    float fCosine = max(dot(u_LightDirection, vDir), 0.0);
#endif

    float shadow = currentDepth - closestDepth > bias ? 0.0 : fCosine;

    result += u_LightColor * vDiffuse /* * NdotL */ * shadow;

    o_LightpassOutput = result;
}
