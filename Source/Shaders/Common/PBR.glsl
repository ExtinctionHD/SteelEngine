#ifndef PBR_GLSL
#define PBR_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#include "Common/Common.glsl"

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float DistributionGGX(float NdotH, float roughness)
{
    const float a = roughness * roughness;
    const float a2 = a * a;
    const float NdotH2 = NdotH * NdotH;

    const float nom = a2;
    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    const float r = roughness + 1.0f;
    const float k = r * r * 0.125f;

    const float nom = NdotV;
    const float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    const float ggxV = GeometrySchlickGGX(NdotV, roughness);
    const float ggxL = GeometrySchlickGGX(NdotL, roughness);

    return ggxV * ggxL;
}

float Specular(float NdotH, float NdotV, float NdotL, float roughness)
{
    const float D = DistributionGGX(NdotH, roughness);
    const float G = GeometrySmith(NdotV, NdotL, roughness);
    
    const float nom = D * G;
    const float denom = 4.0f * NdotV * NdotL + 0.001f;

    return nom / denom;
}

vec3 CalculatePBR(vec3 polygonN, vec3 N, vec3 V, vec3 L, vec3 lightColor, float lightIntensity, vec3 baseColor, float roughness, float metallic, float occlusion, float shadow)
{
    vec3 H = normalize(V + L);

    const float NdotV = max(dot(N, V), 0.0f);
    const float NdotL = max(dot(N, L), 0.0f);
    const float NdotH = max(dot(N, H), 0.0f);
    const float HdotV = max(dot(H, V), 0.0f);

    const vec3 F0 = mix(vec3(0.04), baseColor, metallic);
    const vec3 F = FresnelSchlick(HdotV, F0);

    const vec3 kS = F;
    vec3 kD = (vec3(1.0f) - kS);
    kD *= 1.0f - metallic;

    const vec3 ambient =  0.01f * baseColor * occlusion;
    const vec3 diffuse = kD * baseColor * INVERSE_PI;
    const vec3 specular = kS * Specular(NdotH, NdotV, NdotL, roughness);
    const vec3 illumination = lightColor * lightIntensity * NdotL * (1.0f - shadow);

    return ambient + (diffuse + specular) * illumination;
}

#endif