#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(set = 0, binding = 0) uniform Constants {
    mat4 mproj;
    mat4 mview;
    mat4 mmodel;
    mat4 lightViewProjection;
    vec4 viewpos;
} c;
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec2 inMaterial;
layout(location = 0) out vec3 worldPosition;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec2 uv;
layout(location = 3) out vec2 material;
layout(location = 4) out vec4 shadowCoordinate;
out gl_PerVertex { vec4 gl_Position; };
void main() {
    vec4 world = c.mmodel * vec4(inPosition, 1.0);
    worldPosition = world.xyz;
    worldNormal = normalize(mat3(c.mmodel) * inNormal);
    uv = inUV;
    material = inMaterial;
    shadowCoordinate = c.lightViewProjection * world;
    gl_Position = c.mproj * c.mview * world;
}
