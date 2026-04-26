#version 450

#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "pbr.glsl"

layout(location = 0) in vec2 v2fTexCoord;

layout(scalar, set = 0, binding = 0) uniform UScene {
    mat4 camera;
    mat4 projection;
    mat4 projCam;
    vec4 lightPos;
    vec4 lightColour;
    vec4 cameraPos;
} uScene;

layout(set = 1, binding = 0) uniform sampler2D gTexColour;
layout(set = 1, binding = 1) uniform sampler2D gNormal;
layout(set = 1, binding = 2) uniform sampler2D gRoughnessMetalness;
layout(set = 1, binding = 3) uniform sampler2D gMappedNormal;
layout(set = 1, binding = 4) uniform sampler2D gDepth;
layout(set = 1, binding = 5) uniform samplerCube skybox;
layout(set = 1, binding = 6) uniform sampler2D gAO;

layout(location = 0) out vec4 oColor;

vec3 reconstructWorldPos(vec2 uv) {
    float depth = texture(gDepth, uv).r;

    // texture coords to normalised device coords
    vec3 ndc;
    ndc.x = (uv.x * 2.f) - 1.f;
    ndc.y = (uv.y * 2.f) - 1.f;

    // use depth for Z
    ndc.z = depth;

    // transform to world space
    vec4 worldPos = inverse(uScene.projCam) * vec4(ndc, 1.f);

    return worldPos.xyz / worldPos.w;
}


void main()
{
    float depth = texture(gDepth, v2fTexCoord).r;

    // sky colour for pixels at depth limit
    if (depth >= 1.f) {
        vec2 ndc = (v2fTexCoord * 2.f) - 1.f;

        vec4 clip = vec4(ndc, 1.f, 1.f);
        vec4 view = inverse(uScene.projection) * clip;
        vec3 viewDir = normalize((inverse(uScene.camera) * vec4(view.xyz, 0.f)).xyz);

        oColor = texture(skybox, viewDir);
        return;
    }

    vec3 worldSpace = reconstructWorldPos(v2fTexCoord);

    // Beckman roughness = roughness^2
    // get roughness and metalness from g-buffers
    float roughness = texture(gRoughnessMetalness, v2fTexCoord).r;
    float metalness = texture(gRoughnessMetalness, v2fTexCoord).g;
    vec3 emissive = vec3(0.f);
    vec3 baseColour = texture(gTexColour, v2fTexCoord).rgb;
    vec3 sceneAmbient = vec3(0.15f);

    // normals encoded in g-buffer as [0, 1], reverse this to recover normals in [-1, 1]
    vec3 encodedNormal = texture(gMappedNormal, v2fTexCoord).rgb;
    vec3 normal = normalize((encodedNormal * 2.f) - 1.f);

    // vector pointing towards the camera
    vec3 viewDirection = normalize(uScene.cameraPos.xyz - worldSpace);

    vec3 lightColour = uScene.lightColour.xyz;
    vec3 lightPos = uScene.lightPos.xyz;

    // vector pointing towards the light
    vec3 lightDirection = normalize(lightPos - worldSpace);

    // half vector from the light and view directions
    vec3 halfVector = normalize(lightDirection + viewDirection);

    float nDotLPos = max(dot(normal, lightDirection), 0.05f);

    vec3 brdfResult = brdf(lightDirection, viewDirection, normal, halfVector, roughness, metalness, baseColour);
    vec3 lighting =  brdfResult * lightColour * nDotLPos;

    float ambientOcclusion = texture(gAO, v2fTexCoord).r;
    vec3 finalColour = emissive + (ambient(sceneAmbient, baseColour) * (ambientOcclusion * ambientOcclusion)) + lighting;
    finalColour = clamp(finalColour, 0.f, 1.f);

    oColor = vec4(finalColour, 1.0);
}