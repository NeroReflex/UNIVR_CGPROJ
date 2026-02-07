#version 320 es

precision highp float;

#define USE_PCF_SHADOWS 1

#define SMOOTH_BIAS 1

layout(location = 0) in vec2 v_TexCoord;

uniform sampler2D u_GDiffuse;
uniform sampler2D u_GSpecular;
uniform sampler2D u_GShininess;
uniform sampler2D u_GNormalTangentSpace;
uniform sampler2D u_GPosition;
uniform sampler2D u_GNormal;
uniform sampler2D u_GTangent;

uniform sampler2D u_LightpassInput;

uniform sampler2D u_LDepthTexture;

layout(location = 0) uniform mat4 u_LightSpaceMatrix;

layout(location = 1) uniform vec3 u_LightDir;
layout(location = 2) uniform vec3 u_LightColor;
layout(location = 3) uniform vec3 u_CameraPosition;

layout(location = 0) out vec3 o_LightpassOutput;

#if USE_PCF_SHADOWS
float pcf_shadow(float currentDepth, float bias, ivec2 center) {
    float shadow = 0.0;
    for(int x = -2; x <= 2; ++x) {
        for(int y = -2; y <= 2; ++y) {
            float pcfDepth = texelFetch(u_LDepthTexture, center + ivec2(x, y), 0).r;
            shadow += currentDepth - bias < pcfDepth ? 1.0 : 0.0;
        }    
    }
    shadow /= 9.0;
    return shadow;
}
#endif

void main() {
    vec3 vNormal_worldspace = texture(u_GNormal, v_TexCoord).rgb;
    vec3 vTangent_worldspace = texture(u_GTangent, v_TexCoord).rgb;
    vec3 vBitangent_worldspace = normalize(cross(vNormal_worldspace, vTangent_worldspace));
    mat3 TBN = mat3(vTangent_worldspace, vBitangent_worldspace, vNormal_worldspace);
    mat3 invTBN = transpose(TBN);

    vec3 vDiffuse = texture(u_GDiffuse, v_TexCoord).rgb;
    vec4 vPosition_worldspace = vec4(texture(u_GPosition, v_TexCoord).xyz, 1.0);

    ivec2 shadowTextureDimensions = textureSize(u_LDepthTexture, 0);

    // load the input lightpass color: this is the light accumulated so far
    vec3 result = texture(u_LightpassInput, v_TexCoord).rgb;

    vec3 normal = normalize(texture(u_GNormalTangentSpace, v_TexCoord).xyz /* * 2.0 - 1.0 */);
    vec3 light_dir = normalize(invTBN * u_LightDir);

    vec4 lightSpacePos = u_LightSpaceMatrix * vPosition_worldspace;
    lightSpacePos /= lightSpacePos.w;
    vec2 shadowTexCoord = lightSpacePos.xy * 0.5 + 0.5;
    if (shadowTexCoord.x < 0.0 || shadowTexCoord.x > 1.0 || shadowTexCoord.y < 0.0 || shadowTexCoord.y > 1.0) {
        o_LightpassOutput = result;
        return;
    }

    vec2 texelPos = shadowTexCoord * vec2(float(shadowTextureDimensions.x), float(shadowTextureDimensions.y));

    // accumulate the contribution of the current directional light
    float NdotL_neg = dot(normal, -light_dir);
    float NdotL = max(NdotL_neg, 0.0);

    float closestDepth = texelFetch(u_LDepthTexture, ivec2(texelPos), 0).r;

    float currentDepth = lightSpacePos.z * 0.5 + 0.5;

#if SMOOTH_BIAS
    float bias = max(0.005 * (1.0 - NdotL_neg), 0.0005);
#else
    const float bias = 0.004;
#endif

#if USE_PCF_SHADOWS
    float shadow = pcf_shadow(currentDepth, bias, ivec2(texelPos));
#else
    float shadow = (currentDepth - bias) > closestDepth ? 0.0 : 1.0;
#endif

    vec3 diffuseContrib = u_LightColor * vDiffuse * NdotL;

    // Specular (Blinn-Phong) using G-buffer specular color and shininess
    vec3 specColor = texture(u_GSpecular, v_TexCoord).rgb;
    float shininess = texture(u_GShininess, v_TexCoord).r;
    vec3 viewDir_tangentspace = normalize(invTBN * normalize(u_CameraPosition - vPosition_worldspace.xyz));
    vec3 L = normalize(-light_dir); // toward light in tangent space
    vec3 H = normalize(L + viewDir_tangentspace);
    float specFactor = pow(max(dot(normal, H), 0.0), max(shininess, 1.0));
    vec3 specularContrib = u_LightColor * specColor * specFactor * NdotL;

    result += shadow * (diffuseContrib + specularContrib);

    o_LightpassOutput = result;
}
