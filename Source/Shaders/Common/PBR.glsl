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

vec3 F_Schlick(vec3 F0, vec3 wo, vec3 wm)
{
    return F0 + (1 - F0) * pow(1 - dot(wo, wm), 5);
}

float D_GGX(float a2, float cosWm)
{
    const float d = Pow2(cosWm) * (a2 - 1) + 1;
    return a2 / PI * Pow2(d);
}

float G_SmithGGX(float a, float cosWo, float cosWi)
{
    const float k = a * a * 0.5;

    const float ggxV = cosWo * (1 - k) + k;
    const float ggxL = cosWi * (1 - k) + k;

    return 1 / ggxV * ggxL;
}

vec3 EvaluateBSDF(Surface surface, vec3 wo, vec3 wi, vec3 wm)
{
    const float cosWo = max(dot(surface.N, wo), 0);
    const float cosWi = max(dot(surface.N, wi), 0);
    const float cosWm = max(dot(surface.N, wm), 0);

    const float D = D_GGX(surface.a2, cosWm);
    const vec3 F = F_Schlick(surface.F0, wo, wm);
    const float G = G_SmithGGX(surface.a, cosWo, cosWi);

    vec3 kD = mix(vec3(1) - F, vec3(0.0), surface.metallic);

    const vec3 diffuse = kD * surface.baseColor * INVERSE_PI;
    const vec3 specular = D * F * G * 0.25; // cosWi * cosWo cancel out with G_SmithGGX

    return diffuse + specular;
}

vec3 CalculatePBR(Surface surface, float occlusion, float shadow,
        vec3 wo, vec3 wi, vec3 wm, vec3 lightColor, float lightIntensity)
{
    const vec3 ambient =  0.01 * surface.baseColor * occlusion;
    const vec3 illumination = lightColor * lightIntensity * max(dot(surface.N, wi), 0) * (1 - shadow);

    return ambient + EvaluateBSDF(surface, wo, wi, wm) * illumination;
}

#endif