#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec2 inBoxSize;
layout(location = 4) in vec4 inCornerRadii;
layout(location = 5) in vec4 inBorderColor;
layout(location = 6) in float inBorderWidth;

layout(push_constant) uniform PushConstants {
    mat4 projection;
} uPush;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragUV;

void main() {
    gl_Position = uPush.projection * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
    fragUV = inUV;
}
