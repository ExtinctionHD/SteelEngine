#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE FRAGMENT_STAGE
#pragma shader_stage(fragment)

#include "Common/Common.glsl"

#include "Hybrid/LightVolumePositions.layout"

void main()
{
    vec3 coeffs[SH_COEFFICIENT_COUNT];
    for (uint i = 0; i < SH_COEFFICIENT_COUNT; ++i)
    {
        const uint offset = inIndex * SH_COEFFICIENT_COUNT * 3 + i * 3;

        coeffs[i].r = coefficients[offset + 0];
        coeffs[i].g = coefficients[offset + 1];
        coeffs[i].b = coefficients[offset + 2];
    }
    
    const vec3 result = ComputeIrradiance(coeffs, inNormal);

    outColor = vec4(ToneMapping(result), 1.0);
}