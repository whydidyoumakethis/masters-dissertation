#version 450

#define MAX_LIGHTS 32

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec2 iTexCoord;
layout(location = 2) in vec3 iNormal;
layout(location = 3) in vec4 iTangent;
layout(location = 4) in ivec4 iBoneIDs;
layout(location = 5) in vec4 iWeights;

layout(scalar, set = 0, binding = 0) readonly buffer ShadowMatrices {
    mat4 lightMatrices[];
} shadowMatrices;

layout(scalar, set = 1, binding = 0) uniform BoneMatrices {
    mat4 bones[100];
} uBones;

layout(push_constant) uniform PushConstants {
    mat4 model;
    ivec4 indices; // x is light index, y is face index
} object;

void main() {
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
    int matrixIndex = (object.indices.x * 6) + object.indices.y;
    gl_Position = shadowMatrices.lightMatrices[matrixIndex] * finalModelMat * vec4(iPosition, 1.f);
}
