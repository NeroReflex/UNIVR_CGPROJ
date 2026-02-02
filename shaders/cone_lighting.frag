#version 320 es

precision highp float;

#define SMOOTH_BIAS 1

layout(location = 0) in vec2 v_TexCoord;

uniform sampler2D u_GDiffuse;
uniform sampler2D u_GSpecular;
uniform sampler2D u_GNormalTangentSpace;
uniform sampler2D u_GPosition;
uniform sampler2D u_GNormal;
uniform sampler2D u_GTangent;

uniform sampler2D u_LightpassInput;

uniform sampler2D u_LDepthTexture;

layout(location = 0) uniform mat4 u_LightSpaceMatrix;

layout(location = 1) uniform vec3 u_LightColor;
layout(location = 2) uniform vec3 u_LightPosition;
layout(location = 3) uniform vec3 u_LightDirection;
layout(location = 4) uniform float u_LightCutOff;
layout(location = 5) uniform float u_LightOuterCutOff;
layout(location = 6) uniform float u_LightConstant;
layout(location = 7) uniform float u_LightLinear;
layout(location = 8) uniform float u_LightQuadratic;


layout(location = 0) out vec3 o_LightpassOutput;

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

    // vector from light to point in tangent space
    vec3 vDir_tangentspace = normalize(invTBN * normalize(u_LightPosition - vPosition_worldspace.xyz));
    // cone axis in tangent space
    vec3 vLightDir_tangentspace = normalize(invTBN * u_LightDirection);

    // angle between cone axis and vector from light to point
    float theta = dot(vLightDir_tangentspace, -vDir_tangentspace);

    // smooth inner/outer cone falloff
    float epsilon = u_LightCutOff - u_LightOuterCutOff;
    float coneIntensity = 0.0;
    if (epsilon > 0.0) coneIntensity = clamp((theta - u_LightOuterCutOff) / epsilon, 0.0, 1.0);

    // shadow test (simple hard shadow)
    float shadow = currentDepth - closestDepth > bias ? 0.0 : 1.0;

    // distance attenuation
    float distance = length(u_LightPosition - vPosition_worldspace.xyz);
    float attenuation = 1.0 / (u_LightConstant + u_LightLinear * distance + u_LightQuadratic * (distance * distance));

    // use normal in tangent space for lighting coefficient
    vec3 n_ts = normalize(texture(u_GNormalTangentSpace, v_TexCoord).rgb /* * 2.0 - 1.0*/ );

    float NdotL = max(dot(n_ts, vDir_tangentspace), 0.0);

    // Il modello di default ha distanze troppo elevate
    if (attenuation < 0.001) {
        attenuation = 1.0;
    }

    result += u_LightColor * vDiffuse /* *  NdotL */ *  attenuation * coneIntensity * shadow;

    o_LightpassOutput = result;
}
