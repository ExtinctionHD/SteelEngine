#ifndef PATH_TRACING_GLSL
#define PATH_TRACING_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE raygen
#pragma shader_stage(raygen)
void main() {}
#endif

#include "Common/PBR.glsl"

struct Ray
{
    vec3 origin;
    vec3 direction;
    float TMin;
    float TMax;
};

struct Payload
{
    float hitT;
    vec3 normal;
    vec3 tangent;
    vec2 texCoord;
    uint matId;
};

struct Surface
{
    mat3 TBN;
    vec3 baseColor;
    float roughness;
    float metallic;
    vec3 emission;
    vec3 F0;
    float a;
    float a2;
    float sw;
};

float GetSpecularWeight(vec3 baseColor, vec3 F0, float metallic)
{
    const float diffuseLum = mix(Luminance(baseColor), 0.0, metallic);
    const float specularLum = Luminance(F0);
    return min(1, specularLum / (specularLum + diffuseLum));
}

vec3 EvaluateBSDF(Surface surface, vec3 V, vec3 L, vec3 H)
{
    const float NoV = CosThetaTangent(V);
    const float NoL = CosThetaTangent(L);
    const float NoH = CosThetaTangent(H);
    const float VoH = max(dot(V, H), 0.0);

    const float D = D_GGX(surface.a2, NoH);
    const vec3 F = F_Schlick(surface.F0, VoH);
    const float Vis = Vis_Schlick(surface.a, NoV, NoL);

    vec3 kD = mix(vec3(1.0) - F, vec3(0.0), surface.metallic);

    const vec3 diffuse = kD * Diffuse_Lambert(surface.baseColor);
    const vec3 specular = D * F * Vis;

    return diffuse + specular;
}

float PdfBSDF(Surface surface, vec3 wo, vec3 wi, vec3 wh)
{
    const float diffusePdf = CosinePdfHemisphere(CosThetaTangent(wi));
    const float specularPdf = SpecularPdf(CosThetaTangent(wh), surface.a2, dot(wi, wh));

    return mix(diffusePdf, specularPdf, surface.sw);
}

vec3 SampleBSDF(Surface surface, vec3 wo, out vec3 wi, out float pdf, inout uvec2 seed)
{
    const vec3 E = NextVec3(seed);

    vec3 wh;

    if (E.z < surface.sw)
    {
        wh = ImportanceSampleGGX(E.xy, surface.a2);
        wi = -reflect(wo, wh);
    }
    else
    {
        wi = CosineSampleHemisphere(E.xy);
        wh = normalize(wo + wi);
    }

    pdf = PdfBSDF(surface, wo, wi, wh);
    return EvaluateBSDF(surface, wo, wi, wh);
}

#endif