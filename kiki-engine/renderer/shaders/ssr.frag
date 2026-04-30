#version 450

#define MAX_LIGHTS 8

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

layout(set = 1, binding = 0) uniform sampler2D uSceneColour;
layout(set = 1, binding = 1) uniform sampler2D gTexColour;
layout(set = 1, binding = 2) uniform sampler2D gNormal;
layout(set = 1, binding = 3) uniform sampler2D gRoughnessMetalness;
layout(set = 1, binding = 4) uniform sampler2D gDepth;
layout(set = 1, binding = 5) uniform sampler2D gMappedNormal;

layout(location = 0) out vec4 oColor;

layout(push_constant) uniform PushConstants {
    vec4 settings; // maxSteps, binarySteps, stepSize, thicknessTolerance
} ssrSettings;

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

vec3 reconstructUV(vec3 worldPos) {
    vec4 clip = uScene.projCam * vec4(worldPos, 1.f);

    vec3 ndc = clip.xyz / clip.w;

    vec2 uv;
    uv.x = (ndc.x + 1.f) / 2.f;
    uv.y = (ndc.y + 1.f) / 2.f;

    float depth = ndc.z;

    return vec3(uv, depth);
}


bool isUVOnScreen(vec3 uv) {
    return uv.x >= 0.f && uv.x <= 1.f && uv.y >= 0.f && uv.y <= 1.f;
}

void main()
{
    int ssrMaxSteps = int(ssrSettings.settings.x);
    int ssrBinarySteps = int(ssrSettings.settings.y);
    float ssrStepSize = ssrSettings.settings.z;
    float ssrThicknessTolerance = ssrSettings.settings.w;

    vec3 sceneColour = texture(uSceneColour, v2fTexCoord).rgb;

    float depth = texture(gDepth, v2fTexCoord).r;

    float roughness = texture(gRoughnessMetalness, v2fTexCoord).r;
    float metalness = texture(gRoughnessMetalness, v2fTexCoord).g;

    // skip skybox and non-metallic surfaces
    // skip where maxSteps == 0
    if (depth >= 1.f || metalness <= 0.01f || ssrMaxSteps <= 0) {
        oColor = vec4(sceneColour, 1.f);
        return;
    }

    // decode normal from [0, 1] back to [-1, 1]
    vec3 encodedGeometricNormal = texture(gNormal, v2fTexCoord).rgb;
    vec3 geometricNormal = normalize((encodedGeometricNormal * 2.f) - 1.f);

    vec3 encodedMappedNormal = texture(gMappedNormal, v2fTexCoord).rgb;
    vec3 mappedNormal = normalize((encodedMappedNormal * 2.f) - 1.f);

    vec3 worldPos = reconstructWorldPos(v2fTexCoord);

    vec3 viewDirection = normalize(uScene.cameraPos.xyz - worldPos); // direction from surface to camera
    vec3 reflectDirection = normalize(reflect(-viewDirection, geometricNormal)); // reflect the view direction around the normal

    // skip rays pointing away from the screen
    if (dot(reflectDirection, viewDirection) >= 0.99f) {
        oColor = vec4(sceneColour, 1.f);
        return;
    }

    // iteratively march the reflection ray
    vec3 rayOrigin = worldPos + (reflectDirection * ssrStepSize); // move a little in the reflection direction to prevent self-intersections
    vec3 rayPos = rayOrigin;
    vec3 rayStep = reflectDirection * ssrStepSize;

    bool hit = false;
    vec3 hitUV;
    vec3 prevUV = reconstructUV(worldPos);

    for (int step = 0; step < ssrMaxSteps; step++) {
        vec3 sampleUV = reconstructUV(rayPos);

        // stop if the ray leaves the screen
        if (!isUVOnScreen(sampleUV)) break;

        float sceneDepth = texture(gDepth, sampleUV.xy).r;
        float depthDelta = sampleUV.z - sceneDepth;

        // successful hit! :D
        if (depthDelta > 0.f && depthDelta < ssrThicknessTolerance) {
            // binary search between prevUV and sampleUV to refine hit
            vec3 low = prevUV;
            vec3 high = sampleUV;
            for (int binaryStep = 0; binaryStep < ssrBinarySteps; binaryStep++) {
                vec3 midpoint = 0.5f * (low + high);
                float midpointSceneDepth = texture(gDepth, midpoint.xy).r;
                if (midpoint.z - midpointSceneDepth > 0.f) {
                    high = midpoint;
                }
                else {
                    low = midpoint;
                }
            }

            hit = true;
            hitUV = high;
            break;
        }

        prevUV = sampleUV;
        rayPos += rayStep;
    }

    vec3 reflectionColour = sceneColour;
    float reflectionStrength = 0.f;

    if (hit) {
        reflectionColour = texture(uSceneColour, hitUV.xy).rgb;

        // fade at screen edges so reflections end smoothly
        vec2 edgeFade = smoothstep(vec2(0.f), vec2(0.1f), hitUV.xy) * smoothstep(vec2(0.f), vec2(0.1f), 1.f - hitUV.xy);

        // less reflections on rough surfaces
        float roughnessFade = 1.f - roughness;

        reflectionStrength = edgeFade.x * edgeFade.y * roughnessFade * metalness * clamp(dot(geometricNormal, mappedNormal), 0.f, 1.f);
    }

    // linear interpolation between sceneColour and reflectionColour based on the reflectionStrength
    vec3 finalColour = (sceneColour * (1.f - reflectionStrength)) + (reflectionColour * reflectionStrength);
    oColor = vec4(finalColour, 1.f);
}