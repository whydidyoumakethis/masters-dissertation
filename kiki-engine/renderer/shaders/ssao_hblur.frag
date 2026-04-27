#version 450

#extension GL_EXT_scalar_block_layout : require

#define BLUR_SIZE 2

layout(location = 0) in vec2 v2fTexCoord;

layout(set = 0, binding = 0) uniform sampler2D gAO;

layout(location = 0) out float oAO_HBlur;

void main() {
    vec2 texelSize = 1.f / vec2(textureSize(gAO, 0));

    float result = 0.f;
    for (int i = -BLUR_SIZE; i <= BLUR_SIZE; i++) {
        result += texture(gAO, v2fTexCoord + vec2(texelSize.x * i, 0.f)).r;
    }

    oAO_HBlur = result / ((BLUR_SIZE * 2.f) + 1.f);
}