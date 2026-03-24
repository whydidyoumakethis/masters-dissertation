#version 450

#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "pbr.glsl"

layout(location = 0) in vec2 v2fTexCoord;
layout(location = 1) in vec3 v2fNormal;
layout(location = 2) in vec3 v2fWorldSpace;

layout(set = 1, binding = 0) uniform sampler2D uTexColor;
layout(set = 1, binding = 1) uniform sampler2D uTexRoughnessMetalness;
// layout(set = 1, binding = 2) uniform sampler2D uTexAlphaMask;

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
} object;

layout(location = 0) out vec4 oColor;

void main() {
    // float alpha = texture(uTexAlphaMask, v2fTexCoord).a;

    // if (alpha < 0.5) {
    //     discard;
    // }

    // Beckman roughness = roughness^2
    float roughness = pow(texture(uTexRoughnessMetalness, v2fTexCoord).g, 2.f);
    float metalness = texture(uTexRoughnessMetalness, v2fTexCoord).b;
    vec3 emissive = vec3(0.f);
    vec3 baseColour = texture(uTexColor, v2fTexCoord).rgb;
    vec3 lightColour = uScene.lightColour.xyz;
    vec3 sceneAmbient = vec3(0.08f);
    vec3 normal = normalize(v2fNormal);

    // vector pointing towards the light
    vec3 lightDirection = normalize(uScene.lightPos.xyz - v2fWorldSpace);
    
    // vector pointing towards the camera
    vec3 viewDirection = normalize(uScene.cameraPos.xyz - v2fWorldSpace);

    // half vector from the light and view directions
    vec3 halfVector = normalize(lightDirection + viewDirection + vec3(1e-4));

    float nDotLPos = max(dot(normal, lightDirection), 0.f);

    vec3 brdfResult = brdf(lightDirection, viewDirection, normal, halfVector, roughness, metalness, baseColour);

    vec3 finalColour = emissive + ambient(sceneAmbient, baseColour) + (brdfResult * lightColour * nDotLPos);
    finalColour = clamp(finalColour, 0.f, 1.f);

    oColor = vec4(finalColour.rgb, 1.f);
    // oColor = vec4(finalColour.rgb, alpha);
}