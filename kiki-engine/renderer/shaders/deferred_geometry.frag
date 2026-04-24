#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;
layout(location = 1) in vec3 v2fNormal;
layout(location = 2) in vec3 v2fWorldSpace;
layout(location = 3) in vec4 v2fTangent;

layout(set = 1, binding = 0) uniform sampler2D uTexColor;
layout(set = 1, binding = 1) uniform sampler2D uTexRoughnessMetalness;
layout(set = 1, binding = 2) uniform sampler2D uTexNormalMap;

layout(location = 0) out vec4 gTexColour;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec2 gRoughnessMetalness;
layout(location = 3) out vec4 gMappedNormal;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColour;
    vec4 flags; // sprite, useTexture, roughnessFactor, metallicFactor
} object;

vec3 calculateMappedNormal() {
    vec3 normalMapNormal = texture(uTexNormalMap, v2fTexCoord).rgb;
    normalMapNormal = normalize((normalMapNormal * 2.f) - 1.f);

    // MikkTSpace reconstruction: do not orthogonalize T, do not normalize B.
    vec3 N = normalize(v2fNormal);
    vec3 T = normalize(v2fTangent.xyz);
    vec3 B = cross(N, T) * 1.f;

    mat3 tbn = mat3(T, B, N);

    vec3 mappedNormal = normalize(tbn * normalMapNormal);

    return (mappedNormal * 0.5f) + 0.5f;
}

void main()
{
    // Beckman roughness = roughness^2
    float roughness = pow(texture(uTexRoughnessMetalness, v2fTexCoord).g, 2.f);
    roughness *= object.flags.z;

    float metalness = texture(uTexRoughnessMetalness, v2fTexCoord).b;
    metalness *= object.flags.w;

    vec3 baseColour = texture(uTexColor, v2fTexCoord).rgb;

    // if not useTexture then use the baseColour
    if (object.flags.y == 0) {
        baseColour = object.baseColour.rgb;
    }

    gTexColour = vec4(baseColour, 1.f);

    gNormal = vec4((normalize(v2fNormal) * 0.5f) + 0.5f, 1.f);

    gRoughnessMetalness = vec2(roughness, metalness);
    gMappedNormal = vec4(calculateMappedNormal(), 1.f);
}