// https://64.github.io/tonemapping/

#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;

layout(set = 0, binding = 0) uniform sampler2D uSceneColour;

layout(location = 0) out vec4 oColor;

layout(push_constant) uniform TonemapSettings {
    float maxWhite;
    int isEnabled;
} tonemapSettings;


float luminance(vec3 colour)
{
    // using BT.709 RGB to luminance, Y = 0.2126R + 0.7152G + 0.0722B
    return dot(colour, vec3(0.2126f, 0.7152f, 0.0722f));
}

vec3 changeLuminance(vec3 colour, float targetLuminance) {
    return colour * (targetLuminance / luminance(colour));
}

vec3 reinhard(vec3 colour, float luminance) {
    return colour / (1.f + luminance);
}

vec3 reinhardExtended(vec3 colour, float maxWhite) {
    float l = luminance(colour);
    float maxWhite2 = maxWhite * maxWhite;

    float mappedLuminance = (l * (maxWhite2 + l)) / (maxWhite2 * (1.f + l));
    return changeLuminance(colour, mappedLuminance);
}

void main()
{
    vec3 inColour = texture(uSceneColour, v2fTexCoord).rgb;

    if (tonemapSettings.isEnabled == 0) {
        oColor = vec4(inColour, 1.f);
        return;
    } 

    float inLuminance = luminance(inColour);

    oColor = vec4(reinhardExtended(inColour, tonemapSettings.maxWhite), 1.f);
}