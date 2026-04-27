#version 450

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

layout(scalar, set = 0, binding = 0) uniform UInterface {
    mat4 projection;
} uInterface;

layout(location = 0) out vec2 oUv;

void main() {
    oUv = uv;
    gl_Position = uInterface.projection * vec4(position, 0.0, 1.0);
}