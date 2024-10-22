#version 450
#extension GL_EXT_buffer_reference : require

layout(set = 0, binding = 0) uniform SceneData {
    mat4 viewMatrix;
    mat4 projMatrix;
} sceneData;

layout(location = 0) out vec3 outNormal;

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
    int index;
} PushConstants;

void main()
{
    Vertex vertex = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    mat4 view = mat4(mat3(sceneData.viewMatrix));
    gl_Position = sceneData.projMatrix * view * vec4(vertex.position, 1.0);
    outNormal = normalize(vertex.position);
}
