#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexture;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 texCoord;

/* View projection as view project matrix */
layout(binding = 0, set = 0) uniform ViewProjectionUB {
    mat4 view;
    mat4 projection;
    /* Merged view and matrix */
    mat4 vp;
} vpUB;

/* Push constant for model */
layout(push_constant, std430) uniform ubo {
    mat4 model;
} pushConstant;

void main () {
    gl_Position = vpUB.vp * pushConstant.model * vec4 (inPosition, 1);
    fragColor = vec3(1, 1, 0);
    texCoord = inTexture;
}
