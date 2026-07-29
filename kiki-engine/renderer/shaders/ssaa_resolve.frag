#version 450

layout(location = 0) in vec2 v2fTexCoord;

layout(set = 0, binding = 0) uniform sampler2D uHighResolutionColour;

layout(push_constant) uniform SSAASettings {
    uint scale;
} ssaaSettings;

layout(location = 0) out vec4 oColor;

void main() {
    int scale = int(max(ssaaSettings.scale, 1u));
    ivec2 sourceSize = textureSize(uHighResolutionColour, 0);
    ivec2 outputPixel = ivec2(gl_FragCoord.xy);
    ivec2 sourceOrigin = outputPixel * scale;

    vec3 accumulatedColour = vec3(0.0);
    int sampleCount = 0;

    for (int y = 0; y < scale; ++y) {
        for (int x = 0; x < scale; ++x) {
            ivec2 sourcePixel = clamp(
                sourceOrigin + ivec2(x, y),
                ivec2(0),
                sourceSize - ivec2(1)
            );
            accumulatedColour += texelFetch(uHighResolutionColour, sourcePixel, 0).rgb;
            ++sampleCount;
        }
    }

    oColor = vec4(accumulatedColour / float(sampleCount), 1.0);
}
