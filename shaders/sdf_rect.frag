#version 450

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inLocalPos;
layout(location = 2) in vec2 inBoxSize;
layout(location = 3) in vec4 inCornerRadii;
layout(location = 4) in vec4 inBorderColor;
layout(location = 5) in float inBorderWidth;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 p = inLocalPos;
    vec2 b = inBoxSize * 0.5;

    float r = (p.x >= 0.0) ? ((p.y >= 0.0) ? inCornerRadii.z : inCornerRadii.y)
                           : ((p.y >= 0.0) ? inCornerRadii.w : inCornerRadii.x);

    r = min(r, min(b.x, b.y));

    vec2 q = abs(p) - b + vec2(r);
    float dist = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;

    vec2 dx = dFdx(p);
    vec2 dy = dFdy(p);
    float aa = max(0.70710678 * length(vec2(length(dx), length(dy))), 0.5);

    float outerCov = clamp(0.5 - dist / aa, 0.0, 1.0);
    if (outerCov <= 0.0) {
        discard;
    }

    if (inBorderWidth > 0.0) {
        vec2 inner_b = max(vec2(0.0), b - vec2(inBorderWidth));
        float inner_r = max(0.0, r - inBorderWidth);
        inner_r = min(inner_r, min(inner_b.x, inner_b.y));
        vec2 inner_q = abs(p) - inner_b + vec2(inner_r);
        float innerDist = min(max(inner_q.x, inner_q.y), 0.0) + length(max(inner_q, 0.0)) - inner_r;

        float innerCov = clamp(0.5 - innerDist / aa, 0.0, 1.0);

        if (inColor.a <= 0.0001) {
            float ringCov = (1.0 - innerCov) * outerCov;
            if (ringCov <= 0.0) {
                discard;
            }
            outColor = vec4(inBorderColor.rgb, inBorderColor.a * pow(ringCov, 1.0 / 1.4));
        } else {
            float borderAlpha = inBorderColor.a * (1.0 - innerCov);
            float fillAlpha = inColor.a * innerCov;
            float outAlpha = borderAlpha + fillAlpha;
            vec3 outRgb =
                (inBorderColor.rgb * borderAlpha + inColor.rgb * fillAlpha) / max(outAlpha, 0.0001);
            outColor = vec4(outRgb, outAlpha * pow(outerCov, 1.0 / 1.4));
        }
    } else {
        outColor = vec4(inColor.rgb, inColor.a * pow(outerCov, 1.0 / 1.4));
    }
}
