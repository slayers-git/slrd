#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 texCoord;

// specifies the index of the framebuffer
layout(location = 0) out vec4 outColor;

layout(binding = 1, set = 0)
uniform sampler2D iTexture;

void main () {
    // outColor = vec4 (texCoord.xy, 0, 1);
    // outColor = vec4 (fragColor, 1);

    outColor = texture (iTexture, texCoord);
}
