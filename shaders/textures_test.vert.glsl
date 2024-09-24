#version 450
#extension GL_EXT_buffer_reference : require

layout(set = 0, binding = 0) uniform SceneData {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 viewProjectionMatrix;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
} sceneData;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec2 outUV;

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

    gl_Position = sceneData.viewProjectionMatrix * PushConstants.worldMatrix * vec4(vertex.position, 1.0);
    outColor = vertex.color.xyz;
    outUV = vec2(vertex.uvX, vertex.uvY);
    outNormal = vertex.normal;
}