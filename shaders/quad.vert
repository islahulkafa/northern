#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    vec2 uScale;
    vec2 uTranslate;
} push;

void main() {
    gl_Position = vec4(inPosition * push.uScale + push.uTranslate, 0.0, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
