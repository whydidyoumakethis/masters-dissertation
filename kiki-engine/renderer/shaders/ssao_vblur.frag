#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;

layout(set = 0, binding = 0) uniform sampler2D gAO_HBlur;

layout(location = 0) out float oAO_Blur;

layout(push_constant) uniform SSAOSettings {
    int width;
    int height;
    int samples;
    float radius;
    int blurSize;
} ssaoSettings;

void main() {
    vec2 texelSize = 1.f / vec2(textureSize(gAO_HBlur, 0));

    float result = 0.f;
    for (int i = -ssaoSettings.blurSize; i <= ssaoSettings.blurSize; i++) {
        result += texture(gAO_HBlur, v2fTexCoord + vec2(0.f, texelSize.y * i)).r;
    }

    oAO_Blur = result / ((ssaoSettings.blurSize * 2.f) + 1.f); 
}