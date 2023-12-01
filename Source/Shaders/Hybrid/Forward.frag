#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "Common/Stages.h"
#define SHADER_STAGE FRAGMENT_STAGE
#pragma shader_stage(fragment)

#define ALPHA_TEST 0
#define DOUBLE_SIDED 0
#define NORMAL_MAPPING 0

#define RAY_TRACING_ENABLED 1
#define LIGHT_VOLUME_ENABLED 1

#include "Common/Common.h"
#include "Common/Common.glsl"
#include "Common/PBR.glsl"

#include "Hybrid/Forward.layout"
#include "Hybrid/Material.glsl"
#include "Hybrid/Lighting.glsl"

void main() 
{
    const Material material = materials[materialIndex];

    const vec4 baseColor = GetBaseColor(material);

    #if ALPHA_TEST
        if (baseColor.a < material.alphaCutoff)
        {
            discard;
        }
    #endif

    const vec3 N = GetNormal(material);
    const vec3 emission = GetEmission(material);
    const vec2 roughnessMetallic = GetRoughnessMetallic(material);
    const float occlusion = GetOcclusion(material);

    const vec3 baseColorLinear = ToLinear(baseColor.rgb);
    const vec3 emissionLinear = ToLinear(emission);

    #if DEBUG_OVERRIDE_MATERIAL
        const float roughness = DEBUG_ROUGHNESS;
        const float metallic = DEBUG_METALLIC;
    #else
        const float roughness = roughnessMetallic.r;
        const float metallic = roughnessMetallic.g;
    #endif

    const vec3 F0 = mix(DIELECTRIC_F0, baseColor.rgb, metallic);
    
    const vec3 V = normalize(frame.cameraPosition - inPosition);
    
    const float NoV = CosThetaWorld(N, V);

    const vec3 directLighting = ComputeDirectLighting(inPosition, N, V, NoV, baseColorLinear, F0, roughness, metallic);

    const vec3 indirectLighting = ComputeIndirectLighting(inPosition, N, V, NoV, baseColorLinear, F0, roughness, metallic, occlusion);

    const vec3 result = ToneMapping(indirectLighting + directLighting + emissionLinear);

    outColor = vec4(result, baseColor.a);
}