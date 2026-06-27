#version 450

layout(location = 0) in vec2 inTexCord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 lightDirection;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    outColor = texture(texSampler, inTexCord);
    if (outColor.w <= 0.8) discard;

    outColor.xyz *= pow(outColor.w, 2);

    if (length(lightDirection) != 0.0) {
        float lighting = clamp(
            dot(-fragNormal, normalize(lightDirection)),
            0.2,
            1.0
        );
        outColor.xyz *= lighting;
    }
}