#version 450

layout(location = 0) in vec2 iUv;

layout(push_constant) uniform PushConstants {
    vec4 colour;
    mat4 model;
    vec2 size;
    float radius;
} pc;

layout(location = 0) out vec4 oColour;

float sdrect(vec2 p, vec2 halfsize, float radius) {
    halfsize -= vec2(radius);
    vec2 d = abs(p) - halfsize;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - radius;
}

void main() {
    float distance = sdrect((iUv - 0.5) * pc.size, pc.size * 0.5, pc.radius);

    oColour = vec4(pc.colour.rgb, pc.colour.a * (1.0 - clamp(distance, 0.0, 1.0)));

    if (oColour.a < 0.01) discard;
}