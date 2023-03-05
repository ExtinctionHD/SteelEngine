#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#include "Common/Common.h"
#include "Common/Common.glsl"
#include "Common/PBR.glsl"
#include "Hybrid/Lighting.glsl"

#define ALPHA_TEST 0
#define DOUBLE_SIDED 0
#define NORMAL_MAPPING 0

layout(constant_id = 1) const uint MATERIAL_COUNT = 256;

layout(push_constant) uniform PushConstants{
    layout(offset = 64) vec3 cameraPosition;
    uint materialIndex;
};

layout(set = 1, binding = 0) uniform sampler2D textures[];
layout(set = 1, binding = 1) uniform materialUBO{ Material materials[MATERIAL_COUNT]; };

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
#if NORMAL_MAPPING
    layout(location = 3) in vec3 inTangent;
#endif

layout(location = 0) out vec4 outColor;

void main() 
{
    Material material = materials[materialIndex];
    
    vec4 baseColor = material.baseColorFactor;
    if (material.baseColorTexture >= 0)
    {
        baseColor *= texture(textures[nonuniformEXT(material.baseColorTexture)], inTexCoord);
    }
    
    const vec3 albedo = baseColor.rgb;
    const float alpha = baseColor.a;

    #if ALPHA_TEST
        if (alpha < material.alphaCutoff)
        {
            discard;
        }
    #endif

    #if DOUBLE_SIDED
        const vec3 V = normalize(cameraPosition - inPosition);
        const vec3 polygonN = FaceForward(inNormal, V);
    #else
        const vec3 polygonN = inNormal;
    #endif

    #if NORMAL_MAPPING
        vec3 normalSample = texture(textures[nonuniformEXT(material.normalTexture)], inTexCoord).xyz * 2.0 - 1.0;
        normalSample = normalize(normalSample * vec3(material.normalScale, material.normalScale, 1.0));
        const vec3 N = normalize(TangentToWorld(normalSample, GetTBN(polygonN, inTangent)));
    #else
        const vec3 N = polygonN;
    #endif

    vec3 emission = material.emissionFactor.rgb;
    if (material.emissionTexture >= 0)
    {
        emission *= texture(textures[nonuniformEXT(material.emissionTexture)], inTexCoord).rgb;
    }

    vec2 roughnessMetallic = vec2(material.roughnessFactor, material.metallicFactor);
    if (material.roughnessMetallicTexture >= 0)
    {
        roughnessMetallic *= texture(textures[nonuniformEXT(material.roughnessMetallicTexture)], inTexCoord).gb;
    }

    float occlusion = material.occlusionStrength;
    if (material.occlusionTexture >= 0)
    {
        occlusion *= texture(textures[nonuniformEXT(material.occlusionTexture)], inTexCoord).r;
    }

    #if DEBUG_OVERRIDE_MATERIAL
        const float roughness = DEBUG_ROUGHNESS;
        const float metallic = DEBUG_METALLIC;
    #else
        const float roughness = roughnessMetallic.r;
        const float metallic = roughnessMetallic.g;
    #endif
    
    const vec3 F0 = mix(DIELECTRIC_F0, albedo, metallic);

    const vec3 V = normalize(cameraPosition - inPosition);

    const float NoV = CosThetaWorld(N, V);

    const vec3 directLighting = CalculateDirectLighting(inPosition, V, N, NoV, albedo, F0, roughness, metallic);

    const vec3 indirectLighting = CalculateIndirectLighting(inPosition, V, N, NoV, albedo, F0, roughness, metallic, occlusion);

    const vec3 result = ToneMapping(indirectLighting + directLighting + emission);

    outColor = vec4(result, alpha);
}