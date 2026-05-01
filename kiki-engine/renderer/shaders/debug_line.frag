#version 450

layout(location = 0) in vec4 v2fColour;

layout(location = 0) out vec4 oColor;

void main() {
    oColor = v2fColour;
}