#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTexCoord;
layout(location = 2) in vec3 iNormal;
layout(location = 3) in vec4 iTangent;

layout(scalar, set = 0, binding = 0) uniform UScene {
    mat4 camera;
    mat4 projection;
    mat4 projCam;
    vec4 lightPos;
    vec4 lightColour;
    vec4 cameraPos;
} uScene;


layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColour;
    int sprite;
    int useTexture;
} object;

layout(location = 0) out vec2 v2fTexCoord;
layout(location = 1) out vec3 v2fNormal;
layout(location = 2) out vec3 v2fWorldSpace;
layout(location = 3) out vec4 v2fTangent;

void main() {
    v2fTexCoord = iTexCoord;
    v2fNormal = normalize(transpose(inverse(mat3(object.model))) * iNormal);
    v2fWorldSpace = (object.model * vec4(iPosition, 1.f)).xyz;
    // v2fTangent = normalize(mat3(object.model) * iTangent);

    vec3 T = normalize(mat3(object.model) * iTangent.xyz);
    T = normalize(T - v2fNormal * dot(T, v2fNormal));
    v2fTangent = vec4(T, iTangent.w);

    gl_Position = uScene.projCam * object.model * vec4(iPosition, 1.f);
}