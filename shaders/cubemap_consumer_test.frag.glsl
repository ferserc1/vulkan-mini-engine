#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 fragPos;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform SceneData {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
} sceneData;

layout(set = 1, binding = 0) uniform samplerCube colorTex;

void main()
{
    vec3 camPos = vec3(0.0f, 0.0f, -3.0f);
    vec3 V = normalize(camPos - fragPos);
    vec3 R = reflect(-V, inNormal);

    float lightValue = max(dot(inNormal, sceneData.sunlightDirection.xyz), 0.1f);

    vec3 color = inColor * texture(colorTex, R).xyz;
    vec3 ambient = color * sceneData.ambientColor.xyz;
    
    outFragColor = vec4(color * lightValue * sceneData.sunlightColor.w + ambient, 1.0f);
}
