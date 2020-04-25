#ifndef PBR_GLSL
#define PBR_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#include "Common/Constants.glsl"
#include "Common/Common.glsl"
#include "Common/Random.glsl"
#include "Common/MonteCarlo.glsl"

#define DIELECTRIC_F0 vec3(0.04)

struct Surface
{
    mat3 TBN;
    vec3 baseColor;
    float roughness;
    float metallic;
    vec3 F0;
    float a;
    float a2;
    float sw;
};

float GetSpecularWeight(vec3 baseColor, vec3 F0, float metallic)
{
    const float diffuseLum = mix(Luminance(baseColor), 0, metallic);
    const float specularLum = Luminance(F0);
    return min(1, specularLum / (specularLum + diffuseLum));
}

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
    const float NoV = CosThetaTangent(V);
    const float NoL = CosThetaTangent(L);
    const float NoH = CosThetaTangent(H);
    const float VoH = max(dot(V, H), 0);

    const float D = D_GGX(surface.a2, NoH);
    const vec3 F = F_Schlick(surface.F0, VoH);
    const float G = G_Schlick(surface.a, NoV, NoL);

    vec3 kD = mix(vec3(1) - F, vec3(0.0), surface.metallic);

    const vec3 diffuse = kD * Diffuse_Lambert(surface.baseColor);
    const vec3 specular = D * F * G;

    return diffuse + specular;
}

vec3 ImportanceSampleGGX(vec2 E, float a2)
{
	const float phi = 2 * PI * E.x;
	const float cosTheta = sqrt((1 - E.y) / (1 + (a2 - 1) * E.y));
	const float sinTheta = sqrt(1 - cosTheta * cosTheta);

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

float PdfBSDF(Surface surface, vec3 wo, vec3 wi, vec3 wh)
{
    const float diffusePdf = CosinePdfHemisphere(CosThetaTangent(wi));
    const float specularPdf = ImportancePdfGGX(CosThetaTangent(wh), surface.a2) / max(EPSILON, 4 * dot(wi, wh));
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