#version 450

layout(location = 0) in vec2 v2fTexCoord;

layout(set = 1, binding = 0) uniform sampler2D uTexColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColour;
    uint sprite;
} object;

layout(location = 0) out vec4 oColor;

void main() {
    vec4 tex = texture(uTexColor, v2fTexCoord);

    if (object.sprite == 1) {
        oColor = vec4(tex.rgb, tex.a * object.baseColour.a);
    } else {
        vec3 rgb = mix(object.baseColour.rgb, tex.rgb, tex.a);
        oColor = vec4(rgb, object.baseColour.a);
    }
}