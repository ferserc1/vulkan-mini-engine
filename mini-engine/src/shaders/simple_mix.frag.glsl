#version 450

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput gNormal;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput gPosition;
layout(input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput gColor;

layout(location = 0) out vec4 fragColor;

// Simple pass-through shader
void main() {

    vec3 position = subpassLoad(gPosition).xyz;
    vec3 normal = normalize(subpassLoad(gNormal).xyz);
    vec3 color = subpassLoad(gColor).xyz;
    float alpha = subpassLoad(gColor).w;

    if (alpha < 0.02) {
        discard;
    }

    vec3 ambientColor = vec3(0.2, 0.2, 0.2);
    vec3 directionalColor = vec3(0.4, 0.8, 0.1);
    vec3 lightPosition = vec3(5, 8, 5);

    vec3 lightDirection = normalize(lightPosition - position);
    float directionalIntensity = max(dot(normal, lightDirection), 0.0);
    vec3 lightColor = ambientColor + (directionalColor * directionalIntensity);

    vec3 combinedColor = color * lightColor;
    fragColor = vec4(combinedColor, 1.0);
}
