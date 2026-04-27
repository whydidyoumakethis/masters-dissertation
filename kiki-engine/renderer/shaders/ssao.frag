// shader based on this blog post:
// https://learnopengl.com/Advanced-Lighting/SSAO

#version 450

#define MAX_LIGHTS 32
#define RADIUS 0.5f
#define SAMPLES 16
#define PI 3.14159f

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;

layout(scalar, set = 0, binding = 0) uniform UScene {
    mat4 camera;
    mat4 projection;
    mat4 projCam;
    vec4 lightPos[MAX_LIGHTS];
    vec4 lightColour[MAX_LIGHTS];
    vec4 numLights;
    vec4 cameraPos;
    vec4 ssaoSamples[16];
} uScene;

layout(set = 1, binding = 0) uniform sampler2D gNormals;
layout(set = 1, binding = 1) uniform sampler2D gDepth;

layout(location = 0) out float oAO;

layout(push_constant) uniform SSAOSettings {
    int width;
    int height;
} ssaoSettings;

float rand(vec2 seed) {
    return fract(sin(dot(seed, vec2(12.9898f, 78.233f))) * 43758.5453f);
}

vec3 reconstructViewPos(vec2 uv, mat4 invProjection) {
    float depth = texture(gDepth, uv).r;

    // texture coords to normalised device coords
    vec3 ndc;
    ndc.x = (uv.x * 2.f) - 1.f;
    ndc.y = (uv.y * 2.f) - 1.f;

    // use depth for Z
    ndc.z = depth;

    // transform to world space
    vec4 viewPos = invProjection * vec4(ndc, 1.f);

    return viewPos.xyz / viewPos.w;
}

vec3 reconstructUV(vec3 viewPos) {
    vec4 clip = uScene.projection * vec4(viewPos, 1.f);

    vec3 ndc = clip.xyz / clip.w;

    vec2 uv;
    uv.x = (ndc.x + 1.f) / 2.f;
    uv.y = (ndc.y + 1.f) / 2.f;

    float depth = ndc.z;

    return vec3(uv, depth);
}

void main() {
    float depth = texture(gDepth, v2fTexCoord).r;

    // no ambient occlusion for skybox pixels
    if (depth >= 1.f) {
        oAO = 1.f;
        return;
    }

    mat4 invProjection = inverse(uScene.projection);

    vec3 encodedNormal = texture(gNormals, v2fTexCoord).rgb;
    vec3 worldNormal = normalize((encodedNormal * 2.f) - 1.f);

    // get normal in view space
    vec3 normal = normalize(mat3(uScene.camera) * worldNormal);

    vec3 viewPos = reconstructViewPos(v2fTexCoord, invProjection);

    // pick a random angle, use that to generate a random vector
    // use the pixel coordinate as a seed
    vec2 seed = floor(v2fTexCoord * vec2(ssaoSettings.width, ssaoSettings.height));
    float angle = rand(seed) * 2.f * PI;
    vec3 randomVec = normalize(vec3(cos(angle), sin(angle), 0.f));

    // generate TBN based on the random vector
    vec3 T = normalize(randomVec - (normal * dot(randomVec, normal)));
    vec3 B = normalize(cross(normal, T));
    mat3 TBN = mat3(T, B, normal);

    float occlusion = 0.f;

    for (int i = 0; i < SAMPLES; i++) {
        // get sample position in view space
        vec3 samplePos = viewPos + (TBN * vec3(uScene.ssaoSamples[i]) * RADIUS);

        // get UV from sample position
        vec3 offset = reconstructUV(samplePos);

        // ensure offset is on screen
        if (offset.x < 0.f || offset.x > 1.f || offset.y < 0.f || offset.y > 1.f) {
            continue;
        }

        // get depth of sampled position in view space
        float sampledPosDepth = reconstructViewPos(offset.xy, invProjection).z;

        float distance = abs(viewPos.z - sampledPosDepth);

        // if scene depth in front of sample position then there is some geometry blocking it
        // leading to occlusion!
        // add a small bias to prevent self intersections
        if (sampledPosDepth >= samplePos.z + 0.02f) {
            // weight such that near occluders contribute more
            occlusion += smoothstep(0.f, 1.f, RADIUS / distance);
        }    
    }

    oAO = 1.f - (occlusion / float(SAMPLES));
}