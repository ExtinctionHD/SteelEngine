#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#include "Common/Common.h"
#include "Common/Common.glsl"
#include "Hybrid/Hybrid.glsl"

layout(set = 1, binding = 0) uniform sampler3D irradianceVolume[];

layout(location = 0) in vec3 inCoord;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 coeffs[COEFFICIENT_COUNT];

    for (uint i = 0; i < COEFFICIENT_COUNT; ++i)
    {
        coeffs[i] = texture(irradianceVolume[nonuniformEXT(i)], inCoord + vec3(0.5)).xyz;
    }

    const vec3 irradiance = CalculateIrradiance(coeffs, inNormal);

    outColor = vec4(ToneMapping(irradiance), 1.0);
}