#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;

layout(set = 0, binding = 0) uniform sampler2D uSceneColour;

layout(location = 0) out vec4 oColor;

layout(push_constant) uniform BloomData {
    vec4 data;
} bloomData;

void main()
{
    vec3 inColour = texture(uSceneColour, v2fTexCoord).rgb;

    oColor = vec4(inColour, 1.f);
}