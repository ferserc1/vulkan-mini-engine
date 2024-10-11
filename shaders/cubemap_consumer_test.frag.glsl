#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform SceneData {
    mat4 viewMatrix[6];
    mat4 projectionMatrix;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
} sceneData;

layout(set = 1, binding = 0) uniform samplerCube colorTex;

void main()
{
    float lightValue = max(dot(inNormal, sceneData.sunlightDirection.xyz), 0.1f);

    vec3 color = inColor * texture(colorTex, vec3(inUV, 0.0)).xyz;
    vec3 ambient = color * sceneData.ambientColor.xyz;
    
    outFragColor = vec4(color * lightValue * sceneData.sunlightColor.w + ambient, 1.0f);
}
