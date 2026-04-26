#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;

layout(scalar, set = 0, binding = 0) uniform UScene {
    mat4 camera;
    mat4 projection;
    mat4 projCam;
    vec4 lightPos;
    vec4 lightColour;
    vec4 cameraPos;
} uScene;

layout(set = 1, binding = 0) uniform sampler2D gNormals;
layout(set = 1, binding = 1) uniform sampler2D gDepth;

layout(location = 0) out vec4 oAO;

void main()
{
    float depth = texture(gDepth, v2fTexCoord).r;
    // oColor = vec4(depth, depth, depth, 1.f); // TODO

    oAO = vec4(texture(gNormals, v2fTexCoord).rgb, 1.f);
}


