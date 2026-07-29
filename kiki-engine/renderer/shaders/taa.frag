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
    mat4 previousProjCam;
    mat4 inverseProjCam;
    vec4 taaData; // jitter x, jitter y, history weight, reset history
} uScene;

layout(push_constant) uniform TAASettings {
    int historyMethod;
    float historyweight;
    float varianceGamma;
    float padding;
    } taaSettings;

layout(set = 1, binding = 0) uniform sampler2D uCurrentColour;
layout(set = 1, binding = 1) uniform sampler2D uHistoryColour;
layout(set = 1, binding = 2) uniform sampler2D uDepth;

layout(location = 0) out vec4 oColor;


const int KDOP_AXIS_COUNT = 16;
const float DIAGONAL = 0.70710678f;

const vec3 KDOP_AXES[KDOP_AXIS_COUNT] = vec3[](
    vec3(1, 0, 0),
    vec3(0, 1, 0),
    vec3(0, 0, 1),
    vec3(0.820081, 0.456727, -0.344773),
    vec3(0.540295, 0.829202, 0.143195),
    vec3(0.255800, 0.841084, -0.476597),
    vec3(-0.406935, -0.389062, 0.826459),
    vec3(-0.826708, -0.382923, -0.412219),
    vec3(0.260942, -0.577482, 0.773578),
    vec3(0.254398, 0.637821, 0.726957),
    vec3(0.310900, -0.728083, -0.610930),
    vec3(0.798513, -0.556827, -0.228738),
    vec3(0.673383, -0.163602, -0.720964),
    vec3(-0.813922, 0.369658, -0.448201),
    vec3(0.477650, -0.853722, 0.207384),
    vec3(-0.554854, -0.041550, -0.830910)
);




void neighbourhoodKdopBounds(
    vec2 uv,
    out float minProjection[KDOP_AXIS_COUNT],
    out float maxProjection[KDOP_AXIS_COUNT]
) {
    vec2 texelSize =
        1.0f / vec2(textureSize(uCurrentColour, 0));

    vec2 minUv = texelSize * 0.5f;
    vec2 maxUv = vec2(1.0f) - minUv;

    for (int axis = 0; axis < KDOP_AXIS_COUNT; axis++) {
        minProjection[axis] = 100000.0f;
        maxProjection[axis] = -100000.0f;
    }

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 sampleUv = clamp(
                uv + vec2(x, y) * texelSize,
                minUv,
                maxUv
            );

            vec3 colour =
                texture(uCurrentColour, sampleUv).rgb;

            for (int axis = 0;
                 axis < KDOP_AXIS_COUNT;
                 axis++) {
                float projection =
                    dot(colour, KDOP_AXES[axis]);

                minProjection[axis] = min(
                    minProjection[axis],
                    projection
                );

                maxProjection[axis] = max(
                    maxProjection[axis],
                    projection
                );
            }
        }
    }
}


vec3 clipHistoryToKdop(
    vec3 current,
    vec3 history,
    float minProjection[KDOP_AXIS_COUNT],
    float maxProjection[KDOP_AXIS_COUNT]
) {
    vec3 direction = history - current;
    float amount = 1.0f;

    const float epsilon = 0.00001f;

    for (int axis = 0;
         axis < KDOP_AXIS_COUNT;
         axis++) {
        vec3 projectionAxis = KDOP_AXES[axis];

        float currentProjection =
            dot(current, projectionAxis);

        float directionProjection =
            dot(direction, projectionAxis);

        if (directionProjection > epsilon) {
            float axisAmount =
                (maxProjection[axis]
                    - currentProjection)
                / directionProjection;

            amount = min(amount, axisAmount);
        }
        else if (directionProjection < -epsilon) {
            float axisAmount =
                (minProjection[axis]
                    - currentProjection)
                / directionProjection;

            amount = min(amount, axisAmount);
        }
    }

    return current
        + direction * clamp(amount, 0.0f, 1.0f);
}

vec3 reconstructWorldPos(vec2 uv, float depth) {
    vec3 ndc;
    ndc.x = (uv.x * 2.0f) - 1.0f;
    ndc.y = (uv.y * 2.0f) - 1.0f;
    ndc.z = depth;

    vec4 worldPos = uScene.inverseProjCam * vec4(ndc, 1.0f);
    return worldPos.xyz / worldPos.w;
}

bool isOnScreen(vec2 uv) {
    return uv.x >= 0.0f && uv.x <= 1.0f && uv.y >= 0.0f && uv.y <= 1.0f;
}

void neighbourhoodBounds(vec2 uv, out vec3 minColour, out vec3 maxColour) {
    vec2 texelSize = 1.0f / vec2(textureSize(uCurrentColour, 0));

    vec2 minUv = texelSize * 0.5f;
    vec2 maxUv = vec2(1.0f) - minUv;

    minColour = vec3(100000.0f);
    maxColour = vec3(-100000.0f);

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            
            vec2 sampleUv = clamp(
            uv + (vec2(x, y) * texelSize),
            minUv,
            maxUv);

            vec3 sampleColour = texture(uCurrentColour, sampleUv).rgb;
            minColour = min(minColour, sampleColour);
            maxColour = max(maxColour, sampleColour);
        }
    }
}

void neighbourhoodMoments(
    vec2 uv,
    out vec3 meanColour,
    out vec3 standardDeviation
) {
    vec2 texelSize =
        1.0f / vec2(textureSize(uCurrentColour, 0));

    vec2 minUv = texelSize * 0.5f;
    vec2 maxUv = vec2(1.0f) - minUv;

    vec3 firstMoment = vec3(0.0f);
    vec3 secondMoment = vec3(0.0f);

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 sampleUv = clamp(
                uv + vec2(x, y) * texelSize,
                minUv,
                maxUv
            );

            vec3 colour =
                texture(uCurrentColour, sampleUv).rgb;

            firstMoment += colour;
            secondMoment += colour * colour;
        }
    }

    const float sampleCount = 9.0f;

    meanColour = firstMoment / sampleCount;

    vec3 variance =
        (secondMoment / sampleCount)
        - (meanColour * meanColour);

    // Prevent negative values caused by floating-point error.
    variance = max(variance, vec3(0.0f));

    standardDeviation = sqrt(variance);
}

vec3 clipHistoryToAabbCentre(
    vec3 history,
    vec3 minColour,
    vec3 maxColour
) {
    vec3 centre =
        (minColour + maxColour) * 0.5f;

    vec3 extent = max(
        (maxColour - minColour) * 0.5f,
        vec3(0.00001f)
    );

    vec3 offset = history - centre;
    vec3 normalisedOffset = abs(offset) / extent;

    float maximumComponent = max(
        normalisedOffset.r,
        max(normalisedOffset.g, normalisedOffset.b)
    );

    if (maximumComponent > 1.0f) {
        return centre + offset / maximumComponent;
    }

    return history;
}


vec3 clipHistoryToAabb(
    vec3 current,
    vec3 history,
    vec3 minColour,
    vec3 maxColour
) {
    vec3 direction = history - current;
    float amount = 1.0f;

    for (int channel = 0; channel < 3; channel++) {
        if (abs(direction[channel]) > 0.000001f) {
            float boundary =
                direction[channel] > 0.0f
                    ? maxColour[channel]
                    : minColour[channel];

            float channelAmount =
                (boundary - current[channel])
                / direction[channel];

            amount = min(amount, channelAmount);
        }
    }

    return current +
        direction * clamp(amount, 0.0f, 1.0f);
}

vec3 validateHistory(
    vec3 current,
    vec3 history,
    vec2 uv
) {
    if (taaSettings.historyMethod == 0) {
        return history;
    }

    vec3 minColour;
    vec3 maxColour;

    if(taaSettings.historyMethod == 1 || taaSettings.historyMethod == 2){
        neighbourhoodBounds(
            uv,
            minColour,
            maxColour
        );

        if (taaSettings.historyMethod == 1) {
            return clamp(
                history,
                minColour,
                maxColour
            );
        }

        if (taaSettings.historyMethod == 2) {
            return clipHistoryToAabb(
                current,
                history,
                minColour,
                maxColour
            );
        }
    }

    if(taaSettings.historyMethod == 3) {
        vec3 meanColour;
        vec3 standardDeviation;

        neighbourhoodMoments(
        uv,
        meanColour,
        standardDeviation
        );

        float gammaValue = max(taaSettings.varianceGamma, 0.0f);

        minColour = meanColour - gammaValue * standardDeviation;

        maxColour = meanColour + gammaValue * standardDeviation;

        return clipHistoryToAabbCentre(
            history,
            minColour,
            maxColour
        );

    }

    if(taaSettings.historyMethod == 4) {
        float minProjection[KDOP_AXIS_COUNT];
        float maxProjection[KDOP_AXIS_COUNT];

        neighbourhoodKdopBounds(
            uv,
            minProjection,
            maxProjection
        );

        return clipHistoryToKdop(
            current,
            history,
            minProjection,
            maxProjection
        );
    }

    return history;
}


void main() {


    vec2 currentJitterUv = uScene.taaData.xy * 0.5f;
    vec2 currentUv = clamp(v2fTexCoord - currentJitterUv, vec2(0.0f), vec2(1.0f));
    vec3 currentColour = texture(uCurrentColour, currentUv).rgb;
    float depth = texture(uDepth, currentUv).r;

    if (uScene.taaData.w > 0.5f) {// Reset history
        oColor = vec4(currentColour, 1.0f);
        return;
    }

    if (depth >= 1.0f) {// No geometry, use history colour
        vec3 historyColour = texture(uHistoryColour, v2fTexCoord).rgb;

        historyColour = validateHistory(currentColour, historyColour, currentUv);

        oColor = vec4(mix(currentColour, historyColour, taaSettings.historyweight), 1.0f);
        return;
    }

    vec3 worldPos = reconstructWorldPos(currentUv, depth);
    vec4 previousClip = uScene.previousProjCam * vec4(worldPos, 1.0f);

    if (previousClip.w <= 0.0f) {// Behind camera, use current colour
        oColor = vec4(currentColour, 1.0f);
        return;
    }

    vec2 previousUv = ((previousClip.xy / previousClip.w) * 0.5f) + 0.5f;// Convert from clip space to uv space

    if (!isOnScreen(previousUv)) {// Outside of screen, use current colour
        oColor = vec4(currentColour, 1.0f);
        return;
    }

    vec3 historyColour = validateHistory(currentColour, texture(uHistoryColour, previousUv).rgb, currentUv);

    vec3 resolvedColour = mix(currentColour, historyColour, taaSettings.historyweight);

    oColor = vec4(resolvedColour, 1.0f);
}
