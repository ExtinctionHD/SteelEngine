#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "Common/Stages.h"
#define SHADER_STAGE FRAGMENT_STAGE
#pragma shader_stage(fragment)

#define ALPHA_TEST 0
#define DOUBLE_SIDED 0
#define NORMAL_MAPPING 0
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

    outNormal.rgb = GetNormal(material) * 0.5 + 0.5;
    outSceneColor.rgb = GetEmission(material);
    outBaseColorOcclusion.rgba = vec4(baseColor.rgb, GetOcclusion(material));
    outRoughnessMetallic.rg = GetRoughnessMetallic(material);
}
