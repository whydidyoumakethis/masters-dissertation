#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;

layout(set = 0, binding = 0) uniform sampler2D uSceneColour;
layout(set = 0, binding = 1) uniform sampler2D uBaseColour;
layout(set = 0, binding = 2) uniform sampler2D uMappedNormals;
layout(set = 0, binding = 3) uniform sampler2D uNormals;
layout(set = 0, binding = 4) uniform sampler2D uDepth;
layout(set = 0, binding = 5) uniform sampler2D uRoughnessMetalness;
layout(set = 0, binding = 6) uniform sampler2D uSSAO;
layout(set = 0, binding = 7) uniform sampler2D uBloomColour;

layout(push_constant) uniform DebugSettings {
    int mode;
} debugSettings;

layout(location = 0) out vec4 oColor;

void main()
{
    switch (debugSettings.mode) {
        default:
        case 0: // scene colour
            vec3 inColour = texture(uSceneColour, v2fTexCoord).rgb;
            oColor = vec4(inColour, 1.f);
            break;
        case 1: // texture colour
            vec3 inTexture = texture(uBaseColour, v2fTexCoord).rgb;
            oColor = vec4(inTexture, 1.f);
            break;
        case 2: // mapped normals
            vec3 inMappedNormals = texture(uMappedNormals, v2fTexCoord).rgb;
            oColor = vec4(inMappedNormals, 1.f);
            break;
        case 3: // geometric normals
            vec3 inNormals = texture(uNormals, v2fTexCoord).rgb;
            oColor = vec4(inNormals, 1.f);
            break;
        case 4: // depth
            float inDepth = texture(uDepth, v2fTexCoord).r;
            oColor = vec4(vec3(1.f - inDepth), 1.f);
            break;
        case 5: // metalness
            float inMetalness = texture(uRoughnessMetalness, v2fTexCoord).g;
            oColor = vec4(vec3(inMetalness), 1.f);
            break;
        case 6: // roughness
            float inRoughness = texture(uRoughnessMetalness, v2fTexCoord).r;
            oColor = vec4(vec3(inRoughness), 1.f);
            break;
        case 7: // ssao
            float inSSAO = texture(uSSAO, v2fTexCoord).r;
            oColor = vec4(vec3(inSSAO), 1.f);
            break;
        case 8: // bloom
            vec3 inBloom = texture(uBloomColour, v2fTexCoord).rgb;
            oColor = vec4(inBloom, 1.f);
            break;
    };
}