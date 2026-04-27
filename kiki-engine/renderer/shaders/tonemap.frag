// https://64.github.io/tonemapping/

#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;

layout(set = 0, binding = 0) uniform sampler2D uSceneColour;

layout(location = 0) out vec4 oColor;

float luminance(vec3 colour)
{
    // using BT.709 RGB to luminance, Y = 0.2126R + 0.7152G + 0.0722B
    return dot(colour, vec3(0.2126f, 0.7152f, 0.0722f));
}

vec3 reinhard(vec3 colour, float luminance) {
    return colour / (1.f + luminance);
}

void main()
{
    // TODO: reinhard tonemapping
    vec3 inColour = texture(uSceneColour, v2fTexCoord).rgb;
    float inLuminance = luminance(inColour);

    oColor = vec4(reinhard(inColour, inLuminance), 1.f);
}