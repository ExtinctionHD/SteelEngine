#ifndef HYBRID_GLSL
#define HYBRID_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#include "Common/Common.h"
#include "Common/Common.glsl"

vec3 CalculateIrradiance(vec3 coeffs[COEFFICIENT_COUNT], vec3 N)
{
    const float c1 = 0.429043;
    const float c2 = 0.511664;
    const float c3 = 0.743125;
    const float c4 = 0.886227;
    const float c5 = 0.247708;

    const vec3 irradiance = c1 * coeffs[8].xyz * (Pow2(N.x) - Pow2(N.y)) 
        + c3 * coeffs[6].xyz * Pow2(N.z) + c4 * coeffs[0].xyz - c5 * coeffs[6].xyz
        + 2.0 * c1 * (coeffs[4].xyz * N.x * N.y + coeffs[7].xyz * N.x * N.z + coeffs[5].xyz * N.y * N.z)
        + 2.0 * c2 * (coeffs[3].xyz * N.x + coeffs[1].xyz * N.y + coeffs[2].xyz * N.z);

    return irradiance;
}

#endif