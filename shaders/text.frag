#version 450

layout(set = 0, binding = 0) uniform sampler2D glyphAtlas;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    float coverage = texture(glyphAtlas, fragUV).r;
    float alpha = pow(coverage, 1.0 / 1.4);
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
