#version 450

layout(location = 0) in vec2 v2fTexCoord;
layout(location = 0) out vec4 oColor;

layout(set = 0, binding = 0) uniform sampler2D uSSAA;
layout(set = 0, binding = 1) uniform sampler2D uTAA;

layout(push_constant) uniform DifferenceSettings {
    float amplification;
} settings;

void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    vec3 ssaa = texelFetch(uSSAA, pixel, 0).rgb;
    vec3 taa  = texelFetch(uTAA,  pixel, 0).rgb;

    vec3 difference = abs(ssaa - taa) * settings.amplification;
    oColor = vec4(difference, 1.0);
}