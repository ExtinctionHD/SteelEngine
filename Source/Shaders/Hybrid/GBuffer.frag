#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#include "Common/Common.h"
#include "Common/Common.glsl"

#define ALPHA_TEST 0
#define DOUBLE_SIDED 0
#define NORMAL_MAPPING 0
#define MATERIAL_COUNT 256

layout(push_constant) uniform PushConstants{
    layout(offset = 64) vec3 cameraPosition;
    uint materialIndex;
};

layout(set = 1, binding = 0) uniform sampler2D materialTextures[];
layout(set = 1, binding = 1) uniform materialsUBO{ Material materials[MATERIAL_COUNT]; };

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
#if NORMAL_MAPPING
    layout(location = 3) in vec3 inTangent;
#endif

layout(location = 0) out vec4 gBuffer0;
layout(location = 1) out vec4 gBuffer1;
layout(location = 2) out vec4 gBuffer2;
layout(location = 3) out vec4 gBuffer3;

void main() 
{
    Material material = materials[materialIndex];

    vec4 baseColor = material.baseColorFactor;
    if (material.baseColorTexture >= 0)
    {
        baseColor *= texture(materialTextures[nonuniformEXT(material.baseColorTexture)], inTexCoord);
    }
    
    const vec3 albedo = baseColor.rgb;

    #if ALPHA_TEST
        if (baseColor.a < material.alphaCutoff)
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
        vec3 normalSample = texture(materialTextures[nonuniformEXT(material.normalTexture)], inTexCoord).xyz * 2.0 - 1.0;
        normalSample = normalize(normalSample * vec3(material.normalScale, material.normalScale, 1.0));
        const vec3 normal = normalize(TangentToWorld(normalSample, GetTBN(polygonN, inTangent)));
    #else
        const vec3 normal = polygonN;
    #endif

    vec3 emission = material.emissionFactor.rgb;
    if (material.emissionTexture >= 0)
    {
        emission *= texture(materialTextures[nonuniformEXT(material.emissionTexture)], inTexCoord).rgb;
    }

    vec2 roughnessMetallic = vec2(material.roughnessFactor, material.metallicFactor);
    if (material.roughnessMetallicTexture >= 0)
    {
        roughnessMetallic *= texture(materialTextures[nonuniformEXT(material.roughnessMetallicTexture)], inTexCoord).gb;
    }

    float occlusion = material.occlusionStrength;
    if (material.occlusionTexture >= 0)
    {
        occlusion *= texture(materialTextures[nonuniformEXT(material.occlusionTexture)], inTexCoord).r;
    }

    gBuffer0.rgb = normal.xyz * 0.5 + 0.5;
    gBuffer1.rgb = emission;
    gBuffer2.rgba = vec4(albedo, occlusion);
    gBuffer3.rg = roughnessMetallic;
}