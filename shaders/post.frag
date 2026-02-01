#version 320 es

precision highp float;

layout(location = 0) in vec2 v_TexCoord;

uniform sampler2D u_SrcTex;

layout(location = 0) out vec4 o_FragColor;

void main() {
    o_FragColor = texture(u_SrcTex, v_TexCoord);
}
