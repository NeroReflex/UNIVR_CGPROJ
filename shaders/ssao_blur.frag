#version 320 es

precision highp float;

layout(location = 0) in vec2 v_TexCoord;

uniform sampler2D in_ssao;

layout (location = 0) uniform uint u_noise_dimensions;

layout(location = 0) out float out_ssao;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(in_ssao, 0));
    float result = 0.0;

    int dim = int(u_noise_dimensions);
    int half_noise_dim = dim / 2;

    if (dim <= 0) {
        out_ssao = texture(in_ssao, v_TexCoord).r;
        return;
    }

    for (int x = -half_noise_dim; x < half_noise_dim; ++x) {
        for (int y = -half_noise_dim; y < half_noise_dim; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(in_ssao, v_TexCoord + offset).r;
        }
    }

    float samples = float(dim * dim);
    out_ssao = result / samples;
}
