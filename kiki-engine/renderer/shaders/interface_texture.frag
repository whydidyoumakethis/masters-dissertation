#version 450

layout(location = 0) in vec2 iUv;

layout(set = 1, binding = 0) uniform sampler2D texColour;

layout(push_constant) uniform PushConstants {
    vec4 colour;
} pc;

layout(location = 0) out vec4 oColour;

void main() {
    oColour = texture(texColour, iUv) * pc.colour;
    
    if (oColour.a < 0.01) discard;
}