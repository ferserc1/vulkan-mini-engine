#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outPosition;

layout(set = 0, binding = 0) uniform MatrixBuffer {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normal;
} matrices;

void main() {
    gl_Position = matrices.projection * matrices.view * matrices.model * vec4(position, 1.0);
    outNormal = vec3(matrices.normal * vec4(normal, 1.0));
    outPosition = mat3(matrices.model) * position;
}
