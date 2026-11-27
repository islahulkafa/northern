#version 450

layout(set = 0, binding = 0) uniform sampler2D glyphAtlas;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

float adjustCoverage(float cov, float fgLuma) {
    const float kGamma = 1.8;
    float lightOnDark = pow(cov, 1.0 / kGamma);
    float darkOnLight = 1.0 - pow(1.0 - cov, 1.0 / kGamma);
    float adjusted = mix(darkOnLight, lightOnDark, fgLuma);
    return clamp(adjusted + adjusted * (1.0 - adjusted) * mix(0.28, 0.08, fgLuma), 0.0, 1.0);
}

void main() {
    float coverage = texture(glyphAtlas, fragUV).r;
    float fgLuma = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    float alpha = adjustCoverage(coverage, fgLuma);
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
