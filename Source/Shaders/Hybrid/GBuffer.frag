#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "Common/Stages.h"
#define SHADER_STAGE FRAGMENT_STAGE
#pragma shader_stage(fragment)

#define ALPHA_TEST 0
#define DOUBLE_SIDED 0
#define NORMAL_MAPPING 0
#define MATERIAL_COUNT 256

#include "Common/Common.h"
#include "Common/Common.glsl"

#include "Hybrid/GBuffer.layout"
#include "Hybrid/Material.glsl"

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

    gBuffer0.rgb = N * 0.5 + 0.5;
    gBuffer1.rgb = emission;
    gBuffer2.rgba = vec4(baseColor.rgb, occlusion);
    gBuffer3.rg = roughnessMetallic;
}