#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(set = 0, binding = 0) uniform sampler2D overlayTexture;
layout(location = 0) out vec4 outColor;
void main() {
    vec2 uv = vec2(gl_FragCoord.x / 1920.0, 1.0 - gl_FragCoord.y / 1080.0);
    outColor = texture(overlayTexture, uv);
}
