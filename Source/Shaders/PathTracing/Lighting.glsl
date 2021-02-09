#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#extension GL_EXT_ray_query : require

#ifndef SHADER_STAGE
#define SHADER_STAGE raygen
#pragma shader_stage(raygen)
void main() {}
#endif

#include "Common/Common.h"
#include "Common/RayTracing.glsl"
#include "PathTracing/PathTracing.glsl"

layout(constant_id = 0) const uint POINT_LIGHT_COUNT = 4;

const uint DIRECT_LIGHT_ID = POINT_LIGHT_COUNT;
const uint ENVIRONMENT_LIGHT_ID = POINT_LIGHT_COUNT + 1;
const uint LIGHT_COUNT = POINT_LIGHT_COUNT + 2;

layout(set = 2, binding = 1) uniform lightingBuffer{ 
    PointLight pointLights[POINT_LIGHT_COUNT];
    DirectLight directLight;
};

layout(set = 2, binding = 2) uniform samplerCube environmentMap;
layout(set = 2, binding = 3) uniform samplerCube irradianceMap;

float EstimatePointLight(uint index, Surface surface, vec3 p, vec3 wo)
{
    const vec3 direction = pointLights[index].position.xyz - p;
    const float distanceSquared = dot(direction, direction);

    const vec3 N = surface.TBN[2];
    const vec3 L = normalize(direction);

    const float NoL = min(dot(N, L), 1.0);

    const float luminance = Luminance(pointLights[index].color.rgb);

    return luminance * NoL / distanceSquared;
}

float EstimateDirectLight(Surface surface, vec3 p, vec3 wo)
{
    const vec3 N = surface.TBN[2];
    const vec3 L = normalize(-directLight.direction.xyz);

    const float NoL = min(dot(N, L), 1.0);

    const float luminance = Luminance(directLight.color.rgb);

    return luminance * NoL;
}

float EstimateEnvironementLight(Surface surface, vec3 p, vec3 wo)
{
    const vec3 N = surface.TBN[2];

    const vec3 irradiance = texture(irradianceMap, N).rgb;
    const float luminance = Luminance(irradiance);

    return luminance * 2.0 * PI;
}

float EstimateLight(uint lightId, Surface surface, vec3 p, vec3 wo)
{
    float estimation = 0.0;

    if (lightId < POINT_LIGHT_COUNT)
    {
        estimation = EstimatePointLight(lightId, surface, p, wo);
    }
    else if (lightId == DIRECT_LIGHT_ID)
    {
        estimation = EstimateDirectLight(surface, p, wo);
    }
    else if (lightId == ENVIRONMENT_LIGHT_ID)
    {
        estimation = EstimateEnvironementLight(surface, p, wo);
    }

    return estimation;
}

uint SampleLight(Surface surface, vec3 p, vec3 wo, out vec4 light, inout uvec2 seed)
{
    float irradianceEstimation[LIGHT_COUNT];

    irradianceEstimation[0] = EstimateLight(0, surface, p, wo);
    for (uint lightId = 1; lightId < LIGHT_COUNT; ++lightId)
    {
        irradianceEstimation[lightId] = EstimateLight(lightId, surface, p, wo);
        irradianceEstimation[lightId] += irradianceEstimation[lightId - 1];
    }

    for (uint lightId = 0; lightId < LIGHT_COUNT - 1; ++lightId)
    {
        irradianceEstimation[lightId] /= irradianceEstimation[LIGHT_COUNT - 1];
    }
    irradianceEstimation[LIGHT_COUNT - 1] = 1.0;

    const float randSample = NextFloat(seed);

    uint lightId;
    for (lightId = 0; lightId < LIGHT_COUNT - 1; ++lightId)
    {
        if (randSample < irradianceEstimation[lightId])
        {
            break;
        }
    }

    light.w = irradianceEstimation[lightId];      
    if (lightId > 0)
    {
        light.w -= irradianceEstimation[lightId - 1];
    }

    light.xyz = vec3(0.0);
    if (lightId == ENVIRONMENT_LIGHT_ID)
    {
        light.xyz = CosineSampleHemisphere(NextVec2(seed));
        light.w *= CosinePdfHemisphere(CosThetaTangent(light.xyz));
    }

    return lightId;
}

Ray GenerateLightRay(uint lightId, vec3 lightUV, Surface surface, vec3 p)
{
    Ray ray;
    ray.origin = p;
    ray.TMin = RAY_MIN_T;

    if (lightId < POINT_LIGHT_COUNT)
    {
        const vec3 direction = pointLights[lightId].position.xyz - p;

        ray.TMax = length(direction);
        ray.direction = direction / ray.TMax;
    }
    else if (lightId == DIRECT_LIGHT_ID)
    {
        ray.direction = normalize(-directLight.direction.xyz);
        ray.TMax = RAY_MAX_T;
    }
    else if (lightId == ENVIRONMENT_LIGHT_ID)
    {
        const vec3 wi = lightUV;

        ray.direction = TangentToWorld(wi, surface.TBN);
        ray.TMax = RAY_MAX_T;
    }

    return ray;
}

vec3 EvaluatePointLight(uint index, vec3 p)
{
    const vec3 direction = pointLights[index].position.xyz - p;
    const float distanceSquared = dot(direction, direction);
    
    const vec3 color = pointLights[index].color.rgb;

    return color / distanceSquared;
}

vec3 EvaluateLight(uint lightId, vec3 lightUV, vec3 p)
{
    vec3 irradiance = vec3(0.0);

    if (lightId < POINT_LIGHT_COUNT)
    {
        irradiance = EvaluatePointLight(lightId, p);
    }
    else if (lightId == DIRECT_LIGHT_ID)
    {
        irradiance = directLight.color.rgb;
    }
    else if (lightId == ENVIRONMENT_LIGHT_ID)
    {
        irradiance = texture(environmentMap, lightUV).rgb;
    }

    return irradiance;
}

#endif