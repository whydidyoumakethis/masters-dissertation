#version 450

layout(location = 0) in vec2 v2fTexCoord;

layout(set = 1, binding = 0) uniform sampler2D uSceneColour;

layout(location = 0) out vec4 oColor;

layout(push_constant) uniform FXAASettings {
    float fxaaStrength;
    int isEnabled;
} fxaaSettings;

// edge direction tuning
#define FXAA_STRENGTH fxaaSettings.fxaaStrength
#define FXAA_REDUCE_MULT (0.25f * (1.f / FXAA_STRENGTH))
#define FXAA_REDUCE_MIN (1.f / (FXAA_STRENGTH * 16.f))
#define FXAA_SPAN_MAX FXAA_STRENGTH

float luminance(vec3 colour)
{
    // using BT.709 RGB to luminance, Y = 0.2126R + 0.7152G + 0.0722B
    return dot(colour, vec3(0.2126f, 0.7152f, 0.0722f));
}

void main()
{
    if (fxaaSettings.isEnabled == 0) {
        oColor = vec4(texture(uSceneColour, v2fTexCoord).rgb, 1.f);
        return;
    }

    vec2 texelSize = vec2(textureSize(uSceneColour, 0));

    // sample center pixel and its 4 diagonal neighbours
    //  NW -- NE
    //  -- M  --
    //  SW -- SE
    vec3 rgbM = texture(uSceneColour, v2fTexCoord).rgb;
    vec3 rgbNW = texture(uSceneColour, v2fTexCoord + (vec2(-1.f, -1.f) / texelSize)).rgb;
    vec3 rgbNE = texture(uSceneColour, v2fTexCoord + (vec2(1.f, -1.f) / texelSize)).rgb;
    vec3 rgbSW = texture(uSceneColour, v2fTexCoord + (vec2(-1.f, 1.f) / texelSize)).rgb;
    vec3 rgbSE = texture(uSceneColour, v2fTexCoord + (vec2(1.f, 1.f) / texelSize)).rgb;

    // convert RGB values to luminance
    float lumaM = luminance(rgbM);
    float lumaNW = luminance(rgbNW);
    float lumaNE = luminance(rgbNE);
    float lumaSW = luminance(rgbSW);
    float lumaSE = luminance(rgbSE);

    // get min and max luminance values for this neighbourhood
    float lumaMin = min(lumaM, min(lumaNW, min(lumaNE, min(lumaSW, lumaSE))));
    float lumaMax = max(lumaM, max(lumaNW, max(lumaNE, max(lumaSW, lumaSE))));

    // calculate edge direction
    vec2 edgeDirection;
    edgeDirection.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    edgeDirection.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    // bias added to edgeDirection
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * FXAA_REDUCE_MULT, FXAA_REDUCE_MIN);

    // normalise direction
    float dirMin = min(abs(edgeDirection.x), abs(edgeDirection.y)) + dirReduce;
    edgeDirection = clamp(edgeDirection / dirMin, vec2(-FXAA_SPAN_MAX), vec2(FXAA_SPAN_MAX)) / texelSize;

    // calculate blur values
    vec3 narrowBlur = 0.5f * (
        texture(uSceneColour, v2fTexCoord + (edgeDirection * (-1.f / 6.f))).rgb +
        texture(uSceneColour, v2fTexCoord + (edgeDirection * (1.f / 6.f))).rgb
    );

    vec3 wideBlur = (narrowBlur * 0.5f) + 0.25f * (
        texture(uSceneColour, v2fTexCoord + (edgeDirection * -0.5f)).rgb +
        texture(uSceneColour, v2fTexCoord + (edgeDirection * 0.5f)).rgb
    );

    float wideBlurLuminance = luminance(wideBlur);

    // use wider result as long as it hasn't fallen outside the neighbourhood's luminance range
    if (wideBlurLuminance < lumaMin || wideBlurLuminance > lumaMax) {
        oColor = vec4(narrowBlur, 1.f);
    }
    else {
        oColor = vec4(wideBlur, 1.f);
    }
}