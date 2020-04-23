#ifndef PBR_GLSL
#define PBR_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#include "Common/Constants.glsl"
#include "Common/Common.glsl"

#define DIELECTRIC_F0 vec3(0.04)

struct Surface
{
    vec3 baseColor;
    float roughness;
    float metallic;
    vec3 N;
    vec3 F0;
    float a;
    float a2;
};

vec3 Diffuse_Lambert(vec3 baseColor)
{
    return baseColor * INVERSE_PI;
}

vec3 F_Schlick(vec3 F0, float VoH)
{
    const float Fc = pow(1 - VoH, 5);
    return F0 + (1 - F0) * Fc;
}

float D_GGX(float a2, float NoH)
{
    const float d = (NoH * a2 - NoH) * NoH + 1;
    return a2 / (PI * d * d);
}

float G_Schlick(float a, float NoV, float NoL)
{
    const float k = a * 0.5;

    const float ggxV = NoV * (1 - k) + k;
    const float ggxL = NoL * (1 - k) + k;

    return 0.25 / ggxV * ggxL;
}

vec3 EvaluateBSDF(Surface surface, vec3 V, vec3 L, vec3 H)
{
    const float NoV = max(dot(surface.N, V), 0);
    const float NoL = max(dot(surface.N, L), 0);
    const float NoH = max(dot(surface.N, H), 0);
    const float VoH = max(dot(V, H), 0);

    const float D = D_GGX(surface.a2, NoH);
    const vec3 F = F_Schlick(surface.F0, VoH);
    const float G = G_Schlick(surface.a, NoV, NoL);

    vec3 kD = mix(vec3(1) - F, vec3(0.0), surface.metallic);

    const vec3 diffuse = kD * Diffuse_Lambert(surface.baseColor);
    const vec3 specular = D * F * G;

    return diffuse + specular;
}

vec3 CalculatePBR(Surface surface, float occlusion, float shadow,
        vec3 V, vec3 L, vec3 H, vec3 lightColor, float lightIntensity)
{
    const float NoL = max(dot(surface.N, L), 0);

    const vec3 ambient =  0.01 * surface.baseColor * occlusion;
    const vec3 illumination = lightColor * lightIntensity * NoL * (1 - shadow);

    return ambient + EvaluateBSDF(surface, V, L, H) * illumination;
}

#endif