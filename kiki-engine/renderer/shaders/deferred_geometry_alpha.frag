#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;
layout(location = 1) in vec3 v2fNormal;
layout(location = 2) in vec3 v2fWorldSpace;

layout(set = 1, binding = 0) uniform sampler2D uTexColor;
layout(set = 1, binding = 1) uniform sampler2D uTexRoughnessMetalness;

layout(location = 0) out vec4 gTexColour;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec2 gRoughnessMetalness;
layout(location = 3) out vec4 gWorldPos;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColour;
    vec4 flags; // sprite, useTexture
} object;

void main()
{
    float alpha = texture(uTexColor, v2fTexCoord).a;

    if (alpha < 0.5) {
        discard;
    }

    // Beckman roughness = roughness^2
    float roughness = pow(texture(uTexRoughnessMetalness, v2fTexCoord).g, 2.f);
    float metalness = texture(uTexRoughnessMetalness, v2fTexCoord).b;
    vec3 baseColour = texture(uTexColor, v2fTexCoord).rgb;
    vec3 normal = normalize(v2fNormal);

    // if not useTexture then use the baseColour
    if (object.flags.y == 0) {
        baseColour = object.baseColour.rgb;
    }

    gTexColour = vec4(baseColour, 1.f);
    gNormal = vec4((normalize(normal) * 0.5f) + 0.5, 1.f);
    gRoughnessMetalness = vec2(roughness, metalness);
    gWorldPos = vec4(v2fWorldSpace.rgb, 1.f);
}