#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCord;

layout(location = 0) out vec2 fragTextCord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 lightDirection;

layout(push_constant) uniform pc {
    bool enableShading;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);

    fragTextCord = inTexCord;
    fragNormal = (ubo.view * vec4(inNormal, 0.0f)).xyz;

    if (enableShading) {
        lightDirection = (ubo.view * vec4(inPosition, 1.0f) - vec4(0.0, 0.0, -0.01, 1.0)).xyz;
    } else {
        lightDirection = vec3(0.0);
    }
}