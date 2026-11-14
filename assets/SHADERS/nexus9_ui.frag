#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(set = 0, binding = 0) uniform sampler2D overlayTexture;
layout(location = 0) out vec4 outColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(1920.0, 1080.0);
    outColor = texture(overlayTexture, uv);
}
