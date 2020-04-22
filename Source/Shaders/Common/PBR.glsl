#ifndef PBR_GLSL
#define PBR_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#include "Common/Constants.glsl"
#include "Common/Common.glsl"

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1 - F0) * pow(1 - cosTheta, 5);
}

float DistributionGGX(float cosWh, float a2)
{
    float denom = PI * Pow2(Pow2(cosWh) * (a2 - 1) + 1);

    return a2 / denom;
}

float GeometrySchlickGGX(float cosWo, float k)
{
    const float nom = cosWo;
    const float denom = cosWo * (1 - k) + k;

    return nom / denom;
}

float GeometrySmith(float cosWo, float cosWi, float a)
{
    const float k = a * 0.5;

    const float ggxV = GeometrySchlickGGX(cosWo, k);
    const float ggxL = GeometrySchlickGGX(cosWi, k);

    return ggxV * ggxL;
}

float Specular(float cosWh, float cosWo, float cosWi, float a)
{
    const float D = DistributionGGX(cosWh, Pow2(a));
    const float G = GeometrySmith(cosWo, cosWi, a);
    
    const float nom = D * G;
    const float denom = 4 * cosWo * cosWi + 0.001;

    return nom / denom;
}

vec3 CalculatePBR(vec3 polygonN, vec3 N, vec3 V, vec3 L, vec3 lightColor, float lightIntensity, vec3 baseColor, float roughness, float metallic, float occlusion, float shadow)
{
    vec3 H = normalize(V + L);

    const float cosWo = max(dot(N, V), 0);
    const float cosWi = max(dot(N, L), 0);
    const float cosWh = max(dot(N, H), 0);
    const float HdotV = max(dot(H, V), 0);

    const vec3 F0 = mix(vec3(0.04), baseColor, metallic);
    const vec3 F = FresnelSchlick(HdotV, F0);

    const vec3 kS = F;
    vec3 kD = (vec3(1) - kS);
    kD *= 1 - metallic;

    const vec3 ambient =  0.01 * baseColor * occlusion;
    const vec3 diffuse = kD * baseColor * INVERSE_PI;
    const vec3 specular = kS * Specular(cosWh, cosWo, cosWi, roughness);
    const vec3 illumination = lightColor * lightIntensity * cosWi * (1 - shadow);

    return ambient + (diffuse + specular) * illumination;
}

#endif