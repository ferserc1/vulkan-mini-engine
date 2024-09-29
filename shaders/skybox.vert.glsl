#version 450
#extension GL_EXT_buffer_reference : require

layout(set = 0, binding = 0) uniform SceneData {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 viewProjectionMatrix;
} sceneData;

layout(location = 0) out vec2 outUV;

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
    mat4 normalMatrix;
    VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
    Vertex vertex = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    mat4 viewMatrix = mat4(mat3(sceneData.viewMatrix));
    mat4 worldMatrix = mat4(mat3(PushConstants.worldMatrix));
    gl_Position = sceneData.projectionMatrix * viewMatrix * worldMatrix * vec4(vertex.position, 1.0);
    outUV = vec2(vertex.uvX, vertex.uvY);
}
