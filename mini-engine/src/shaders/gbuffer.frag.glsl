#version 450

layout(location = 0) out vec4 outNormal;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outAlbedo;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inPosition;

void main() {
    outNormal = vec4(inNormal, 1.0);
    outPosition = vec4(inPosition, 1.0);
    outAlbedo = vec4(1.0, 1.0, 1.0, 1.0);
}