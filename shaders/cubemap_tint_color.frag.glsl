#version 450

layout(location = 0) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 0) uniform samplerCube inputCubeMap;

layout(set = 2, binding = 0) uniform TintColor {
    vec3 tint;
} tintColor;

void main()
{
    vec3 color = texture(inputCubeMap, inNormal).xyz * tintColor.tint;
    outFragColor = vec4(color, 1.0f);
}
