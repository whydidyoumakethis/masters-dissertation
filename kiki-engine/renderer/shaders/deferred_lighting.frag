#version 450

#extension GL_EXT_scalar_block_layout : require

#define NUM_LIGHTS 4

layout(location = 0) in vec2 v2fTexCoord;

layout(scalar, set = 0, binding = 0) uniform UScene {
    mat4 camera;
    mat4 projection;
    mat4 projCam;
    vec4 lightPos[NUM_LIGHTS];
    vec4 lightColour[NUM_LIGHTS];
    vec4 cameraPos;
} uScene;

layout(set = 1, binding = 0) uniform sampler2D gTexColour;
layout(set = 1, binding = 1) uniform sampler2D gNormal;
layout(set = 1, binding = 2) uniform sampler2D gRoughnessMetalness;
layout(set = 1, binding = 3) uniform sampler2D gDepth;

layout(location = 0) out vec4 oColor;

vec3 reconstructWorldPos(vec2 uv) {
    float depth = texture(gDepth, uv).r;

    // texture coords to normalised device coords
    vec3 ndc;
    ndc.x = (uv.x * 2.f) - 1.f;
    ndc.y = (uv.y * 2.f) - 1.f;

    // use depth for Z
    ndc.z = (depth * 2.f) - 1.f;

    // transform to world space
    vec4 worldPos = inverse(uScene.projCam) * vec4(ndc, 1.f);

    return worldPos.xyz / worldPos.w;
}

void main()
{
    float depth = texture(gDepth, v2fTexCoord).r;

    // sky colour for pixels at depth limit
    if (depth >= 1.0) {
        vec3 skyColour = vec3(0.1f, 0.1f, 0.1f);
        oColor = vec4(skyColour, 1.f);
        return;
    }

    vec3 worldSpace = reconstructWorldPos(v2fTexCoord);
    vec3 baseColour = texture(gTexColour, v2fTexCoord).rgb;

    oColor = vec4(baseColour, 1.f);
}