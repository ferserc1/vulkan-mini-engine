#version 450
#extension GL_EXT_buffer_reference : require

layout(set = 0, binding = 0) uniform ProjectionData {
    mat4 view[6];
    mat4 proj;
} projectionData;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out flat int outCurrentMipLevel;
layout (location = 2) out flat int outTotalMipLevels;

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
    VertexBuffer vertexBuffer;
    int currentFace;
    int currentMipLevel;
    int totalMipLevels;
} PushConstants;

void main()
{
    Vertex vertex = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    int currentFace = PushConstants.currentFace;
    mat4 view = mat4(mat3(projectionData.view[currentFace]));

    gl_Position = projectionData.proj * view * vec4(vertex.position, 1.0);
    outNormal = normalize(vertex.position);
    outCurrentMipLevel = PushConstants.currentMipLevel;
    outTotalMipLevels = PushConstants.totalMipLevels;
}
