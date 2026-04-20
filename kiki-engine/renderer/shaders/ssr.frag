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

layout(set = 1, binding = 0) uniform sampler2D uSceneColour;
layout(set = 1, binding = 1) uniform sampler2D gTexColour;
layout(set = 1, binding = 2) uniform sampler2D gNormal;
layout(set = 1, binding = 3) uniform sampler2D gRoughnessMetalness;
layout(set = 1, binding = 4) uniform sampler2D gDepth;

layout(location = 0) out vec4 oColor;

void main()
{
    // float roughness = texture(gRoughnessMetalness, v2fTexCoord).r; // TODO
    oColor = vec4(texture(uSceneColour, v2fTexCoord).rgb, 1.f);
}