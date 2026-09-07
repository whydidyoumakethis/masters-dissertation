#version 450

#define MAX_LIGHTS 8

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
    vec4 lightPos[MAX_LIGHTS];
    vec4 lightColour[MAX_LIGHTS];
    vec4 numLights;
    vec4 cameraPos;
    vec4 ssaoSamples[16];
	mat4 previousProjCam;
	vec4 taaData;
	mat4 currentUnjitteredProjCam;
} uScene;

layout(scalar, set = 2, binding = 0) uniform BoneMatrices {
    mat4 bones[100];
	mat4 previousBones[100];
} uBones;

layout(push_constant, scalar) uniform PushConstants {
    mat4x3 model;
    mat4x3 previousModel;
    vec4 baseColour;
    vec4 flags;
} object;

layout(location = 0) out vec2 v2fTexCoord;
layout(location = 1) out vec3 v2fNormal;
layout(location = 2) out vec3 v2fWorldSpace;
layout(location = 3) out vec4 v2fTangent;
layout(location = 4) out vec4 v2fCurrentClip;
layout(location = 5) out vec4 v2fPreviousClip;

void main() {
    v2fTexCoord = iTexCoord;

    mat4 skinMat = mat4(0.0);
	mat4 previousSkinMat = mat4(0.0);
    float weightSum = iWeights.x + iWeights.y + iWeights.z + iWeights.w;

    if (weightSum > 0.0) {
        skinMat += iWeights.x * uBones.bones[iBoneIDs.x];
        skinMat += iWeights.y * uBones.bones[iBoneIDs.y];
        skinMat += iWeights.z * uBones.bones[iBoneIDs.z];
        skinMat += iWeights.w * uBones.bones[iBoneIDs.w];
		previousSkinMat += iWeights.x * uBones.previousBones[iBoneIDs.x];
		previousSkinMat += iWeights.y * uBones.previousBones[iBoneIDs.y];
		previousSkinMat += iWeights.z * uBones.previousBones[iBoneIDs.z];
		previousSkinMat += iWeights.w * uBones.previousBones[iBoneIDs.w];
    } else {
        skinMat = mat4(1.0);
		previousSkinMat = mat4(1.0);
    }

	mat4x3 finalModelMat = object.model * skinMat;
	mat4x3 previousFinalModelMat = object.previousModel * previousSkinMat;

    vec4 localPosition = vec4(iPosition, 1.0);

    vec3 currentWorld = finalModelMat * localPosition;
    vec3 previousWorld = previousFinalModelMat * localPosition;


    v2fNormal = normalize(transpose(inverse(mat3(finalModelMat))) * iNormal);
    
    v2fWorldSpace = currentWorld;

	vec3 T = normalize(mat3(finalModelMat) * iTangent.xyz);
    T = normalize(T - v2fNormal * dot(T, v2fNormal));
    v2fTangent = vec4(T, iTangent.w);

    v2fCurrentClip = uScene.currentUnjitteredProjCam * vec4(currentWorld, 1.0);

    v2fPreviousClip = uScene.previousProjCam * vec4(previousWorld, 1.0);

	gl_Position = uScene.projCam * vec4(currentWorld, 1.0);
}