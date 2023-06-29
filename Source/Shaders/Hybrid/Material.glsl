#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#ifndef SHADER_STAGE
    #include "Common/Stages.h"
    #define SHADER_STAGE FRAGMENT_STAGE
    #pragma shader_stage(fragment)
    void main() {}
#endif

#include "Common/Common.h"

#ifndef SHADER_LAYOUT
    #include "Hybrid/GBuffer.layout"
#endif

vec4 GetBaseColor(Material material)
{
    vec4 baseColor = material.baseColorFactor;
    if (material.baseColorTexture >= 0)
    {
        baseColor *= texture(materialTextures[nonuniformEXT(material.baseColorTexture)], inTexCoord);
    }

    return baseColor;
}

vec3 GetNormal(Material material)
{
#if DOUBLE_SIDED
    const vec3 V = normalize(frame.cameraPosition - inPosition);
    const vec3 polygonN = FaceForward(inNormal, V);
#else
    const vec3 polygonN = inNormal;
#endif

#if NORMAL_MAPPING
    vec3 normalSample = texture(materialTextures[nonuniformEXT(material.normalTexture)], inTexCoord).xyz * 2.0 - 1.0;
    normalSample = normalize(normalSample * vec3(material.normalScale, material.normalScale, 1.0));
    return normalize(TangentToWorld(normalSample, GetTBN(polygonN, inTangent)));
#else
    return polygonN;
#endif
}

vec3 GetEmission(Material material)
{
    vec3 emission = material.emissionFactor.rgb;
    if (material.emissionTexture >= 0)
    {
        emission *= texture(materialTextures[nonuniformEXT(material.emissionTexture)], inTexCoord).rgb;
    }

    return emission;
}

vec2 GetRoughnessMetallic(Material material)
{
    vec2 roughnessMetallic = vec2(material.roughnessFactor, material.metallicFactor);
    if (material.roughnessMetallicTexture >= 0)
    {
        roughnessMetallic *= texture(materialTextures[nonuniformEXT(material.roughnessMetallicTexture)], inTexCoord).gb;
    }

    return roughnessMetallic;
}

float GetOcclusion(Material material)
{
    float occlusion = material.occlusionStrength;
    if (material.occlusionTexture >= 0)
    {
        occlusion *= texture(materialTextures[nonuniformEXT(material.occlusionTexture)], inTexCoord).r;
    }

    return occlusion;
}

#endif