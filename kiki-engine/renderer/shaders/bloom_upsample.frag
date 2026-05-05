// https://advances.realtimerendering.com/s2014/index.html

#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 v2fTexCoord;

layout(set = 0, binding = 0) uniform sampler2D uSceneColour;

layout(location = 0) out vec4 oColor;

layout(push_constant) uniform BloomData {
    vec4 data;
} bloomData;

void main()
{
    // original slides mention that we don't scale in terms of pixels
    // instead, scale in terms of a bloom radius
    // from original slides: 'the filter does not map to pixels, it has 'holes' in it'
    float bloomRadius_x = bloomData.data.x;
    float bloomRadius_y = bloomData.data.y;

    // sample 9 texels, around the center
    // these are scaled by the relevant bloom radii
    // NW  N NE
    //  W  C  E
    // SW  S SE
    vec3 C = texture(uSceneColour, v2fTexCoord).rgb;

    vec3 NW = texture(uSceneColour, v2fTexCoord + vec2(-bloomRadius_x, bloomRadius_y)).rgb;
    vec3 N = texture(uSceneColour, v2fTexCoord + vec2(0.f, bloomRadius_y)).rgb;
    vec3 NE = texture(uSceneColour, v2fTexCoord + vec2(bloomRadius_x, bloomRadius_y)).rgb;

    vec3 W = texture(uSceneColour, v2fTexCoord + vec2(-bloomRadius_x, 0.f)).rgb;
    vec3 E = texture(uSceneColour, v2fTexCoord + vec2(bloomRadius_x, 0.f)).rgb;

    vec3 SW = texture(uSceneColour, v2fTexCoord + vec2(-bloomRadius_x, -bloomRadius_y)).rgb;
    vec3 S = texture(uSceneColour, v2fTexCoord + vec2(0.f, -bloomRadius_y)).rgb;
    vec3 SE = texture(uSceneColour, v2fTexCoord + vec2(bloomRadius_x, -bloomRadius_y)).rgb;

    // 3x3 tent filter
    // 1/16 [1 2 1]
    //      [2 4 2]
    //      [1 2 1]
    // maps to:
    // 1/16 [    NW  N * 2     NE ]
    //      [ W * 2  C * 4  E * 2 ]
    //      [    SW  S * 2     SE ]
    N = N * 2.f;
    W = W * 2.f;
    E = E * 2.f;
    S = S * 2.f;

    C = C * 4.f;

    oColor = vec4((NW + N + NE + W + C + E + SW + S + SE) / 16.f, 1.f);

    // this is fast, repeated convolutions converge to Gaussian!

    // naive version:
    // vec3 inColour = texture(uSceneColour, v2fTexCoord).rgb;
    // oColor = vec4(inColour, 1.f);
}