#version 450

layout (location = 0) in vec2 inUV;
layout (location = 1) in flat int inCurrentMipLevel;
layout (location = 2) in flat int inTotalMipLevels;

layout (location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform sampler2D colorTex;

void main()
{
    int notUsed = inCurrentMipLevel + inTotalMipLevels;
    outFragColor = vec4(texture(colorTex, inUV).xyz, 1.0f);
}
