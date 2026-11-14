#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(set = 0, binding = 0) uniform ClearColor { vec4 color; } clearData;
layout(location = 0) out vec4 outColor;
void main() { outColor = clearData.color; }
