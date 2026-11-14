#version 450
#extension GL_ARB_separate_shader_objects : enable
struct PointLight { vec3 Position; float Falloff; vec3 Color; float ColorIntensity; };
layout(set = 0, binding = 12) uniform Constants {
    mat4 mproj;
    mat4 mview;
    mat4 mmodel;
    vec4 viewpos;
    PointLight lights[2];
} c;
layout(set = 0, binding = 0) uniform sampler2D albedoTexture;
layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec2 material;
layout(location = 0) out vec4 outColor;
const float PI = 3.14159265359;
float distributionGGX(float NoH, float roughness) {
    float a = roughness * roughness, a2 = a * a;
    float d = NoH * NoH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 0.00001);
}
float geometrySchlick(float NoV, float roughness) {
    float r = roughness + 1.0, k = (r * r) / 8.0;
    return NoV / max(NoV * (1.0 - k) + k, 0.00001);
}
vec3 fresnelSchlick(float cosTheta, vec3 f0) {
    return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}
vec3 evaluateLight(PointLight light, vec3 albedo, vec3 N, vec3 V, float metallic, float roughness) {
    vec3 delta = light.Position - worldPosition;
    float distanceSq = max(dot(delta, delta), 0.01);
    vec3 L = delta * inversesqrt(distanceSq);
    vec3 H = normalize(V + L);
    float NoV = max(dot(N, V), 0.001);
    float NoL = max(dot(N, L), 0.0);
    float NoH = max(dot(N, H), 0.0);
    float attenuation = 1.0 / distanceSq;
    float edge = clamp(1.0 - distanceSq * light.Falloff, 0.0, 1.0);
    attenuation *= edge * edge;
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float D = distributionGGX(NoH, roughness);
    float G = geometrySchlick(NoV, roughness) * geometrySchlick(NoL, roughness);
    vec3 specular = D * G * F / max(4.0 * NoV * max(NoL, 0.001), 0.001);
    vec3 diffuse = (1.0 - F) * (1.0 - metallic) * albedo / PI;
    return (diffuse + specular) * light.Color * light.ColorIntensity * attenuation * NoL;
}
void main() {
    vec3 albedo = texture(albedoTexture, uv).rgb;
    vec3 N = normalize(worldNormal);
    vec3 V = normalize(c.viewpos.xyz - worldPosition);
    float metallic = clamp(material.x, 0.0, 1.0);
    float roughness = clamp(material.y, 0.045, 1.0);
    vec3 color = albedo * 0.035;
    color += evaluateLight(c.lights[0], albedo, N, V, metallic, roughness);
    color += evaluateLight(c.lights[1], albedo, N, V, metallic, roughness);
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));
    outColor = vec4(color, 1.0);
}
