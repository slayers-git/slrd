#version 460 core

layout(location = 0) out vec4 outResult;
layout(location = 0) in vec4 inColor;

void main () {
    outResult = vec4 (inColor.rgb, 1);
}
