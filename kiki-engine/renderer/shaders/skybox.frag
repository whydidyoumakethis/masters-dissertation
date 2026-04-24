#version 450

#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 v2fDirection;

layout(set = 1, binding = 0) uniform samplerCube skybox;

layout(scalar, set = 0, binding = 0) uniform UScene {
    mat4 camera;
    mat4 projection;
    mat4 projCam;
    vec4 lightPos;
    vec4 lightColour;
    vec4 cameraPos;
} uScene;

layout(location = 0) out vec4 oColor;

void main() {
    // oColor = texture(skybox, vec3(1,0,0));
    // oColor = vec4(1.f, 0.f, 0.f, 1.f);
    oColor = texture(skybox, normalize(v2fDirection));
    // oColor = vec4(normalize(v2fDirection) * 0.5 + 0.5, 1.0);
}