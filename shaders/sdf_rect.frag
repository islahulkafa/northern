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

    float aa = fwidth(dist);
    float outerAlpha = clamp(0.5 - dist / max(aa, 0.0001), 0.0, 1.0);

    if (outerAlpha <= 0.0) {
        discard;
    }

    if (inBorderWidth > 0.0) {
        float innerDist = dist + inBorderWidth;
        float innerAlpha = clamp(0.5 - innerDist / max(aa, 0.0001), 0.0, 1.0);

        vec4 col = mix(inBorderColor, inColor, innerAlpha);
        float finalAlpha = mix(inBorderColor.a, inColor.a, innerAlpha) * outerAlpha;
        outColor = vec4(col.rgb, finalAlpha);
    } else {
        outColor = vec4(inColor.rgb, inColor.a * outerAlpha);
    }
}
