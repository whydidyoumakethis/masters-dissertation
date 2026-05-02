#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;

layout(set = 0, binding = 0) uniform sampler2D uSceneColour;

layout(location = 0) out vec4 oColor;

layout(push_constant) uniform CustomPostprocessSettings {
    vec4 params;
    uint isEnabled;
} customPostprocessSettings;

void main()
{
    if (customPostprocessSettings.isEnabled == 0) {
        oColor = vec4(texture(uSceneColour, v2fTexCoord).rgb, 1.f);
        return;
    }

    vec2 texSize = vec2(textureSize(uSceneColour, 0));
    float blockSize = 12.0;

    vec2 blockCoord = floor(v2fTexCoord * texSize / blockSize) * blockSize;
    vec2 sampleUV = (blockCoord + vec2(blockSize * 0.5)) / texSize;

    oColor = vec4(texture(uSceneColour, sampleUV).rgb, 1.f);
}
