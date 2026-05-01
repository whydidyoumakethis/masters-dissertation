#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec4 iColour;

layout(scalar, set = 0, binding = 0) uniform UScene {
    mat4 camera;
    mat4 projection;
    mat4 projCam;
    vec4 lightPos;
    vec4 lightColour;
    vec4 cameraPos;
} uScene;

layout(location = 0) out vec4 v2fColour;

void main() {
    v2fColour = iColour;
    gl_Position = uScene.projCam * vec4(iPosition, 1.0);
}