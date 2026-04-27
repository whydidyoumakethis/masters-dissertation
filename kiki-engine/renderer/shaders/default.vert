#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTexCoord;
layout(location = 2) in vec3 iNormal;
layout(location = 3) in vec4 iTangent;

layout(location = 4) in ivec4 iBoneIDs;
layout(location = 5) in vec4 iWeights;

layout(scalar, set = 0, binding = 0) uniform UScene {
    mat4 camera;
    mat4 projection;
    mat4 projCam;
    vec4 lightPos;
    vec4 lightColour;
    vec4 cameraPos;
} uScene;

layout(scalar, set = 2, binding = 0) uniform BoneMatrices {
    mat4 bones[100];
} uBones;

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

    mat4 skinMat = mat4(0.0);
    float weightSum = iWeights.x + iWeights.y + iWeights.z + iWeights.w;

    if (weightSum > 0.0) {
        skinMat += iWeights.x * uBones.bones[iBoneIDs.x];
        skinMat += iWeights.y * uBones.bones[iBoneIDs.y];
        skinMat += iWeights.z * uBones.bones[iBoneIDs.z];
        skinMat += iWeights.w * uBones.bones[iBoneIDs.w];
    } else {
        skinMat = mat4(1.0);
    }

    mat4 finalModelMat = object.model * skinMat;

    v2fNormal = normalize(transpose(inverse(mat3(finalModelMat))) * iNormal);
    
    v2fWorldSpace = (finalModelMat * vec4(iPosition, 1.0)).xyz;

    vec3 T = normalize(mat3(object.model) * iTangent.xyz);
    T = normalize(T - v2fNormal * dot(T, v2fNormal));
    v2fTangent = vec4(T, iTangent.w);

    gl_Position = uScene.projCam * finalModelMat * vec4(iPosition, 1.f);
}