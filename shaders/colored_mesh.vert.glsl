#version 450
#extension GL_EXT_buffer_reference : require

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUV;

struct Vertex {
    vec3 position;
    float uvX;
    vec3 normal;
    float uvY;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(push_constant) uniform constants {
    mat4 worldMatrix;
    VertexBuffer vertexBuffer;
} PushConstants;

void main() {
    Vertex vertex = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    gl_Position = PushConstants.worldMatrix * vec4(vertex.position, 1.0);
    outColor = vertex.color.xyz;
    outUV = vec2(vertex.uvX, vertex.uvY);
}