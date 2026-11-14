#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform SceneConstants {
    mat4 viewProjection;
    mat4 model;
    vec4 cameraPosition;
    vec4 lightPosition[2];
    vec4 lightColor[2];
} scene;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec2 inMaterial;

layout(location = 0) out vec3 worldPosition;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec2 uv;
layout(location = 3) out vec2 material;

out gl_PerVertex { vec4 gl_Position; };

void main() {
    vec4 world = scene.model * vec4(inPosition, 1.0);
    worldPosition = world.xyz;
    worldNormal = normalize(mat3(scene.model) * inNormal);
    uv = inUV;
    material = inMaterial;
    gl_Position = scene.viewProjection * world;
}
