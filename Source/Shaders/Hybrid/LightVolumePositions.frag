#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#include "Common/Common.glsl"
#include "Hybrid/Hybrid.glsl"

layout(set = 1, binding = 0) readonly buffer Coefficients{ float coefficients[]; };

layout(location = 0) in flat uint inIndex;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 coeffs[COEFFICIENT_COUNT];
    for (uint i = 0; i < COEFFICIENT_COUNT; ++i)
    {
        const uint offset = inIndex * COEFFICIENT_COUNT * 3 + i * 3;

        coeffs[i].r = coefficients[offset + 0];
        coeffs[i].g = coefficients[offset + 1];
        coeffs[i].b = coefficients[offset + 2];
    }
    
    const vec3 result = ComputeIrradiance(coeffs, inNormal);

    outColor = vec4(ToneMapping(result), 1.0);
}