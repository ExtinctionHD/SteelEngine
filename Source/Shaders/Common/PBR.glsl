#ifndef PBR_GLSL
#define PBR_GLSL

#ifndef SHADER_STAGE
    #define SHADER_STAGE vertex
    #pragma shader_stage(vertex)
    void main() {}
#endif

#include "Common/Constants.glsl"
#include "Common/Common.glsl"
#include "Common/Debug.glsl"
#include "Common/Random.glsl"
#include "Common/MonteCarlo.glsl"

#define DIELECTRIC_F0 vec3(0.04)

vec3 Diffuse_Lambert(vec3 baseColor)
{
    return baseColor * INVERSE_PI;
}

// Microfacet specular = D*F*G / (4*NoL*NoV) = D*F*Vis
// Vis = G / (4*NoL*NoV)

float D_GGX(float a2, float NoH)
{
    const float d = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / (PI * d * d);
}

vec3 F_Schlick(vec3 F0, float VoH)
{
    const float Fc = Pow5(1 - VoH);
    return F0 + (1 - F0) * Fc;
}

vec3 F_SchlickRoughness(vec3 F0, float VoH, float roughness)
{
    const float Fc = Pow5(1 - VoH);
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * Fc;
}

float Vis_Schlick(float a, float NoV, float NoL)
{
    const float k = a * 0.5;

    const float Vis_SchlickV = NoV * (1.0 - k) + k;
    const float Vis_SchlickL = NoL * (1.0 - k) + k;

    return 0.25 * Rcp(Vis_SchlickV * Vis_SchlickL);
}

vec3 ImportanceSampleGGX(vec2 E, float a2)
{
    const float phi = 2.0 * PI * E.x;
    const float cosTheta = sqrt((1.0 - E.y) / (1.0 + (a2 - 1.0) * E.y));
    const float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;
    
    return H;
}

float ImportancePdfGGX(float cosTheta, float a2)
{
    return cosTheta * D_GGX(a2, cosTheta);
}

float SpecularPdf(float NoH, float a2, float VoH)
{
    return ImportancePdfGGX(NoH, a2) / max(4.0 * VoH, EPSILON);
}

vec3 ComposeBRDF(vec3 diffuse, vec3 specular)
{
    #if DEBUG_VIEW_DIFFUSE && DEBUG_VIEW_SPECULAR
        return diffuse + specular;
    #elif DEBUG_VIEW_DIFFUSE
        return diffuse;
    #elif DEBUG_VIEW_SPECULAR
        return specular;
    #else
        return vec3(0.0);
    #endif
}

#endif