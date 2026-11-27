#version 450

layout(set = 0, binding = 0) uniform sampler2D glyphAtlas;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0, index = 0) out vec4 outColor;
layout(location = 0, index = 1) out vec4 outCoverage;

vec3 adjustCoverage(vec3 cov, float fgLuma) {
    const float kGamma = 1.8;
    vec3 lightOnDark = pow(cov, vec3(1.0 / kGamma));
    vec3 darkOnLight = vec3(1.0) - pow(vec3(1.0) - cov, vec3(1.0 / kGamma));
    vec3 adjusted = mix(darkOnLight, lightOnDark, fgLuma);
    return clamp(adjusted + adjusted * (vec3(1.0) - adjusted) * mix(0.28, 0.08, fgLuma), 0.0, 1.0);
}

void main() {
    vec3 rawCov = texture(glyphAtlas, fragUV).rgb;
    float fgLuma = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    vec3 adjCov = adjustCoverage(rawCov, fgLuma) * fragColor.a;
    float maxCov = max(max(adjCov.r, adjCov.g), adjCov.b);
    outColor = vec4(fragColor.rgb, maxCov);
    outCoverage = vec4(adjCov, maxCov);
}
