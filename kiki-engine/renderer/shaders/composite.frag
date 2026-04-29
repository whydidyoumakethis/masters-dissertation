#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;

layout(set = 0, binding = 0) uniform sampler2D uSceneColour;
layout(set = 0, binding = 1) uniform sampler2D uBloomColour;

layout(location = 0) out vec4 oColor;

void main()
{
    vec3 inColour = texture(uSceneColour, v2fTexCoord).rgb;
    vec3 inBloom = texture(uBloomColour, v2fTexCoord).rgb;

    // bloom set to a low value
    // so only bright pixels bloom noticeably
    vec3 colour = inColour + (inBloom * 0.04f);

    oColor = vec4(colour, 1.f);
    oColor = vec4(inBloom * 0.04f, 1.f);
}