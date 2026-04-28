#version 450

#define MAX_LIGHTS 8

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec3 iPosition;

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

layout(location = 0) out vec3 v2fDirection;

void main() {
    // remove translation so that the skybox doesn't move with the camera
    mat4 camera = mat4(mat3(uScene.camera));
    gl_Position = (uScene.projection * camera * vec4(iPosition, 1.0)).xyww;

    v2fDirection = iPosition;
}
