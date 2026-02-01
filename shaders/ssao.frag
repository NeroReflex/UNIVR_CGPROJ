#version 320 es

precision highp float;
precision highp usampler2D;

//#define BIAS_FACTOR 0.0000001
#define HEMISPHERE_SAMPLING 1

layout(location = 0) in vec2 v_TexCoord;

uniform sampler2D u_GNormal;
uniform sampler2D u_GPosition;
uniform usampler2D u_NoiseTex;

layout(location = 0) out float o_ssao;

layout(std430, binding = 0) buffer u_SSAOSamples {
    vec4 samples[];
};

layout (location = 0) uniform uint u_SSAOSampleCount;

layout (location = 1) uniform mat4 u_ViewMatrix;

layout (location = 2) uniform mat4 u_ProjectionMatrix;

layout (location = 3) uniform float u_radius;

layout (location = 4) uniform uint u_noise_dimensions;

#ifndef _RANDOM_
#define _RANDOM_

/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

// Generate a random unsigned int from two unsigned int values, using 16 pairs
// of rounds of the Tiny Encryption Algorithm. See Zafar, Olano, and Curtis,
// "GPU Random Numbers via the Tiny Encryption Algorithm"
uint tea(uint val0, uint val1)
{
  uint v0 = val0;
  uint v1 = val1;
  uint s0 = 0u;

  for(uint n = 0u; n < 16u; n++)
  {
    s0 += 0x9e3779b9u;
    v0 += ((v1 << 4) + 0xa341316cu) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4u);
    v1 += ((v0 << 4) + 0xad90777du) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761eu);
  }

  return v0;
}

// Generate a random unsigned int in [0, 2^24) given the previous RNG state
// using the Numerical Recipes linear congruential generator
uint lcg(inout uint prev)
{
  uint LCG_A = 1664525u;
  uint LCG_C = 1013904223u;
  prev       = (LCG_A * prev + LCG_C);
  return prev & 0x00FFFFFFu;
}

// Generate a random float in [0, 1) given the previous RNG state
float rnd(inout uint prev)
{
  return (float(lcg(prev)) / float(0x01000000));
}

#endif // _RANDOM_

// Returns true if the out_normal is below the horizon defined by surface_normal
//
// This function normalizes out_normal, and out_normal MUST NOT be zero
// and be a direction pointing OUTWARD from the surface
bool is_below_horizon(in vec3 surface_normal, in vec3 out_normal) {
    return dot(surface_normal, normalize(out_normal)) < 0.0;
}

// Given a surface point and  its normal, finds if the other_point is below the horizon
// defined by the surface normal.
bool is_point_below_horizon(vec3 surface_point, vec3 surface_normal, vec3 other_point) {
    // here calculate a vector going from surface_point to other_point,
    // this way we will have an OUTGOING direction from the surface,
    // and we can then use the is_below_horizon function
    vec3 out_normal = other_point - surface_point;

    return is_below_horizon(surface_normal, out_normal);
}

mat4 rotate(float angle, vec3 axis) {
    float c = cos(angle);
    float s = sin(angle);
    float one_c = 1.0 - c;
    
    axis = normalize(axis);
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    return mat4(
        c + x * x * one_c,        x * y * one_c - z * s,  x * z * one_c + y * s,  0.0,
        y * x * one_c + z * s,   c + y * y * one_c,      y * z * one_c - x * s,  0.0,
        z * x * one_c - y * s,   z * y * one_c + x * s,  c + z * z * one_c,      0.0,
        0.0,                      0.0,                      0.0,                      1.0
    );
}

void main() {
    vec3 n = normalize(texture(u_GNormal, v_TexCoord).rgb);
    vec4 position = vec4(texture(u_GPosition, v_TexCoord).xyz, 1.0);

    vec2 noise_scale = vec2(1.0);
    uint noise_seed = 0u;
    if (u_noise_dimensions > 0u) {
        ivec2 noise_tex_size = ivec2(int(u_noise_dimensions), int(u_noise_dimensions));
        ivec2 gbuffer_tex_size = textureSize(u_GPosition, 0);
        noise_scale = vec2(float(gbuffer_tex_size.x) / float(noise_tex_size.x),
                                float(gbuffer_tex_size.y) / float(noise_tex_size.y));

        noise_seed = texture(u_NoiseTex, v_TexCoord * noise_scale).r;
    }

    mat4 vp_matrix = u_ProjectionMatrix * u_ViewMatrix;

    float occlusion = 0.0;
    float validSamples = 0.0;

    // easy way to disable SSAO
    if (u_SSAOSampleCount == 0u) {
        o_ssao = 1.0;
        return;
    }

    for (uint i = 0u; i < u_SSAOSampleCount; ++i) {
        vec4 rotated_sample = vec4(samples[i].xyz * u_radius, 1.0);
        if (u_noise_dimensions > 0u) {
            float rnd_x = rnd(noise_seed);
            float rnd_y = rnd(noise_seed);

            mat4 rot_x = rotate(rnd_x * 6.28318530718, vec3(1.0, 0.0, 0.0));
            mat4 rot_y = rotate(rnd_y * 6.28318530718, vec3(0.0, 1.0, 0.0));

            // apply random rotation to the sample vector
            rotated_sample = vec4((rot_x * rot_y * vec4(samples[i].xyz * u_radius, 0.0)).xyz, 0.0);
        }

        vec3 rejectable_samplePos = position.xyz + rotated_sample.xyz;

        vec3 corrected_samplePos = position.xyz + rotated_sample.xyz;
#if defined(HEMISPHERE_SAMPLING)
        if (is_point_below_horizon(position.xyz, n, rejectable_samplePos)) {
            corrected_samplePos = position.xyz + (-1.0 * rotated_sample.xyz);
        }
#endif

        vec4 samplePos = vec4(corrected_samplePos, 1.0);

        vec4 scaled_samplePos = vp_matrix * samplePos;
        // perspective divide
        scaled_samplePos.xyz /= scaled_samplePos.w;
        // transform to range 0.0 - 1.0 
        scaled_samplePos.xyz = scaled_samplePos.xyz * 0.5 + 0.5;

        // out-of-bound check: skip samples outside the screen
        if (
            (scaled_samplePos.x < 0.0) ||
            (scaled_samplePos.x >= 1.0) ||
            (scaled_samplePos.y < 0.0) ||
            (scaled_samplePos.y >= 1.0)
        ) {
            continue;
        }

        // count this as a valid sample
        validSamples += 1.0;

        // fetch the world-space sample position from the gbuffer, then transform
        // both the current fragment position and the sampled position into
        // view-space so depth comparison is with respect to the camera.
        vec3 sampleWorldPos = texture(u_GPosition, scaled_samplePos.xy).xyz;
        vec4 sampleViewPos = u_ViewMatrix * vec4(sampleWorldPos, 1.0);
        vec4 fragViewPos = u_ViewMatrix * position;

        float sampleDepth = texture(u_GPosition, scaled_samplePos.xy).z;
        float rangeCheck = smoothstep(0.0, 1.0, u_radius / abs(position.z - sampleDepth));

#if defined(BIAS_FACTOR)
        const float bias = BIAS_FACTOR;
        if ((sampleViewPos.z - fragViewPos.z) <= bias) {
            occlusion += rangeCheck;
        }
#else
        if (sampleViewPos.z >= fragViewPos.z) {
            occlusion += rangeCheck;
        }
#endif
    }

    // if no valid samples were inside the screen, assume no occlusion
    if (validSamples == 0.0) {
        o_ssao = 1.0;
    } else {
        // invert so that 1.0 = no occlusion, 0.0 = fully occluded
        o_ssao = 1.0 - (occlusion / validSamples);
    }
}
