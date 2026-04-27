#version 450

layout(location = 0) in vec2 iUv;

layout(set = 1, binding = 0) uniform sampler2D fontAtlas;

layout(push_constant) uniform PushConstants {
    vec4 colour;
} pc;

layout(location = 0) out vec4 oColour;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msd = texture(fontAtlas, iUv).rgb;
    
    float sd = median(msd.r, msd.g, msd.b);
    
    float screenPxDistance = 4.0 * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

    oColour = vec4(pc.colour.rgb, pc.colour.a * opacity);
    
    if (oColour.a < 0.01) discard;
}