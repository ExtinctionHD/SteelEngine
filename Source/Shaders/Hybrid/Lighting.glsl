#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#ifndef SHADER_STAGE
    #define SHADER_STAGE vertex
    #pragma shader_stage(vertex)
    void main() {}
#endif

#ifndef LIGHTING_SET_INDEX
#define LIGHTING_SET_INDEX 0
#endif

#include "Common/Common.h"
#include "Common/Common.glsl"
#include "Common/Debug.glsl"
#include "Common/PBR.glsl"
#include "Common/RayTracing.glsl"

#if RAY_TRACING_ENABLED
    #include "Hybrid/RayQuery.glsl"
#endif

#if LIGHT_VOLUME_ENABLED
    #include "Hybrid/LightVolume.glsl"
#endif

#if LIGHT_COUNT > 0
    layout(set = LIGHTING_SET_INDEX, binding = 0) uniform lightBuffer{ Light lights[LIGHT_COUNT]; };
#endif
layout(set = LIGHTING_SET_INDEX, binding = 1) uniform samplerCube irradianceMap;
layout(set = LIGHTING_SET_INDEX, binding = 2) uniform samplerCube reflectionMap;
layout(set = LIGHTING_SET_INDEX, binding = 3) uniform sampler2D specularBRDF;
// layout(set = LIGHTING_SET_INDEX, binding = 4...6) located in Hybrid/LightVolume.glsl

// layout(set = RAY_QUERY_SET_INDEX, binding = 0...4) located in Hybrid/RayQuery.glsl

float TraceShadowRay(Ray ray)
{
    #if RAY_TRACING_ENABLED
        return IsMiss(TraceRay(ray)) ? 0.0 : 1.0;
    #else
        return 0.0;
    #endif
}

vec3 CalculateDirectLighting(vec3 position, vec3 V, vec3 N, float NoV, vec3 albedo, vec3 F0, float roughness, float metallic)
{
#if DEBUG_VIEW_DIRECT_LIGHTING && LIGHT_COUNT > 0
    vec3 directLighting = vec3(0.0);
    for (uint i = 0; i < LIGHT_COUNT; ++i)
    {
        const Light light = lights[i];

        const float a = roughness * roughness;
        const float a2 = a * a;
        
        const vec3 direction = light.location.xyz - position * light.location.w;

        const float distance = Select(RAY_MAX_T, length(direction), light.location.w);
        const float attenuation = Select(1.0, Rcp(Pow2(distance)), light.location.w);

        const vec3 L = normalize(direction);
        const vec3 H = normalize(L + V);

        const float NoL = CosThetaWorld(N, L);
        const float NoH = CosThetaWorld(N, H);
        const float VoH = max(dot(V, H), 0.0);

        const float irradiance = attenuation * NoL * Luminance(light.color.rgb);

        if (irradiance > EPSILON)
        {
            const float D = D_GGX(a2, NoH);
            const vec3 F = F_Schlick(F0, VoH);
            const float Vis = Vis_Schlick(a, NoV, NoL);
            
            const vec3 kD = mix(vec3(1.0) - F, vec3(0.0), metallic);
            
            const vec3 diffuse = kD * Diffuse_Lambert(albedo);
            const vec3 specular = D * F * Vis;

            Ray ray;
            ray.origin = position + N * BIAS;
            ray.direction = L;
            ray.TMin = RAY_MIN_T;
            ray.TMax = distance;
            
            const float shadow = TraceShadowRay(ray);

            const vec3 lighting = NoL * light.color.rgb * (1.0 - shadow) * attenuation;

            directLighting += ComposeBRDF(diffuse, specular) * lighting;
        }
    }
    return directLighting;
#else
    return vec3(0.0);
#endif
}

vec3 CalculateIndirectLighting(vec3 position, vec3 V, vec3 N, float NoV, vec3 albedo, vec3 F0, float roughness, float metallic, float occlusion)
{
#if DEBUG_VIEW_INDIRECT_LIGHTING
    #if LIGHT_VOLUME_ENABLED
        const vec3 irradiance = SampleLightVolume(position, N);
        const vec3 envIrradiance = texture(irradianceMap, N).rgb;
        const vec3 specularNorm = irradiance / envIrradiance;
    #else
        const vec3 irradiance = texture(irradianceMap, N).rgb;
        const vec3 specularNorm = vec3(1.0);
    #endif

    const vec3 kS = F_SchlickRoughness(F0, NoV, roughness);
    const vec3 kD = mix(vec3(1.0) - kS, vec3(0.0), metallic);

    const vec3 R = -reflect(V, N);
    const float lod = roughness * (textureQueryLevels(reflectionMap) - 1);
    const vec3 reflection = textureLod(reflectionMap, R, lod).rgb;

    const vec2 scaleOffset = texture(specularBRDF, vec2(NoV, roughness)).xy;
    
    const vec3 diffuse = kD * irradiance * albedo;
    const vec3 specular = (F0 * scaleOffset.x + scaleOffset.y) * reflection;
    
    return ComposeBRDF(diffuse, specular * specularNorm) * occlusion;
#else
    return vec3(0.0);
#endif
}

#endif