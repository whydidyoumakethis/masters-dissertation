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
    // compute texel sizes
    vec2 texelSize = 1.f / vec2(textureSize(uSceneColour, 0));
    float texelSize_x = texelSize.x;
    float texelSize_2x = texelSize.x * 2.f;
    float texelSize_y = texelSize.y;
    float texelSize_2y = texelSize.y * 2.f;

    // sample 13 texels around the center
    // TL  TC  TR  (top)
    //   UL  UR    (upper)
    // ML  MC  MR  (middle)
    //   LL  LR    (lower)
    // BL  BC  BR  (bottom)

    vec3 TL = texture(uSceneColour, v2fTexCoord + vec2(-texelSize_2x, texelSize_2y)).rgb;
    vec3 TC = texture(uSceneColour, v2fTexCoord + vec2(0.f, texelSize_2y)).rgb;
    vec3 TR = texture(uSceneColour, v2fTexCoord + vec2(texelSize_2x, texelSize_2y)).rgb;

    vec3 UL = texture(uSceneColour, v2fTexCoord + vec2(-texelSize_x, texelSize_y)).rgb;
    vec3 UR = texture(uSceneColour, v2fTexCoord + vec2(texelSize_x, texelSize_y)).rgb;

    vec3 ML = texture(uSceneColour, v2fTexCoord + vec2(-texelSize_2x, 0.f)).rgb;
    vec3 MC = texture(uSceneColour, v2fTexCoord).rgb;
    vec3 MR = texture(uSceneColour, v2fTexCoord + vec2(texelSize_2x, 0.f)).rgb;

    vec3 LL = texture(uSceneColour, v2fTexCoord + vec2(-texelSize_x, -texelSize_y)).rgb;
    vec3 LR = texture(uSceneColour, v2fTexCoord + vec2(texelSize_x, -texelSize_y)).rgb;

    vec3 BL = texture(uSceneColour, v2fTexCoord + vec2(-texelSize_2x, -texelSize_2y)).rgb;
    vec3 BC = texture(uSceneColour, v2fTexCoord + vec2(0.f, -texelSize_2y)).rgb;
    vec3 BR = texture(uSceneColour, v2fTexCoord + vec2(texelSize_2x, -texelSize_2y)).rgb;

    // then, get weighted averages of 5 sets of 4 samples
    // (the 4 samples around each NW, NE, SW, SE, C)
    // TL  TC  TR
    //   NW  NE
    // ML   C  MR
    //   SW  SE
    // BL  BC  BR

    // 4 samples per box so * 0.25
    vec3 NW = 0.25f * (TL + TC + ML + MC);
    vec3 NE = 0.25f * (TC + TR + MC + MR);
    vec3 SW = 0.25f * (ML + MC + BL + BC);
    vec3 SE = 0.25f * (MC + MR + BC + BR);
    vec3 C = 0.25f * (UL + UR + LL + LR);

    // from the original slides:
    // weight by 0.125 for NW, NE, SW, SE
    // weight by 0.5 for C
    // leads to (0.125 * 4) + 0.5 = 1
    NW = NW * 0.125f;
    NE = NE * 0.125f;
    SW = SW * 0.125f;
    SE = SE * 0.125f;
    C = C * 0.5f;

    // finally, output the result of these 5 weighted averages
    vec3 result = NW + NE + SW + SE + C;
    result = max(result, 0.0001f);

    oColor = vec4(NW + NE + SW + SE + C, 1.f);


    // naive version:
    // vec3 inColour = texture(uSceneColour, v2fTexCoord).rgb;
    // oColor = vec4(inColour, 1.f);
}