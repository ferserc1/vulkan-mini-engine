#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 0) uniform samplerCube skyTexture;

void main()
{
    outFragColor = texture(skyTexture, inNormal);
}
