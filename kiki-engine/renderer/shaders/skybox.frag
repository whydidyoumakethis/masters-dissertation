#version 450

#define MAX_LIGHTS 8

#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 v2fDirection;

layout(set = 1, binding = 0) uniform samplerCube skybox;

layout(scalar, set = 0, binding = 0) uniform UScene {
    mat4 camera;
    mat4 projection;
    mat4 projCam;
    vec4 lightPos[MAX_LIGHTS];
    vec4 lightColour[MAX_LIGHTS];
    vec4 numLights;
    vec4 cameraPos;
    vec4 ssaoSamples[16];
} uScene;

layout(location = 0) out vec4 oColor;

void main() {
    // oColor = texture(skybox, vec3(1,0,0));
    // oColor = vec4(1.f, 0.f, 0.f, 1.f);
    oColor = texture(skybox, normalize(v2fDirection));
    // oColor = vec4(normalize(v2fDirection) * 0.5 + 0.5, 1.0);
}