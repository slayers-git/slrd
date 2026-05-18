#version 460 core

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec4 aColor;

layout(location = 0) out vec4 outColor;

void main () {
    gl_PointSize = 2.f;
    gl_Position = vec4 (aPosition.xy, 0, 1);
    outColor = aColor;
}
