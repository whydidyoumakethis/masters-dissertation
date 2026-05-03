#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;

layout(set = 0, binding = 0) uniform sampler2D uSceneColour;

layout(push_constant) uniform ChromaticAberrationSettings {
    uint isEnabled;
    vec2 redShift;
    vec2 greenShift;
    vec2 blueShift;
} chromaticAberrationSettings;

layout(location = 0) out vec4 oColor;

void main()
{
    if (chromaticAberrationSettings.isEnabled == 0) {
        vec3 inColour = texture(uSceneColour, v2fTexCoord).rgb;
        oColor = vec4(inColour, 1.f);
        return;
    }

    float red = texture(uSceneColour, v2fTexCoord + chromaticAberrationSettings.redShift).r;
    float green = texture(uSceneColour, v2fTexCoord + chromaticAberrationSettings.greenShift).g;
    float blue = texture(uSceneColour, v2fTexCoord + chromaticAberrationSettings.blueShift).b;

    oColor = vec4(red, green, blue, 1.f);
}