#ifndef LIGHT_VOLUME_GLSL
#define LIGHT_VOLUME_GLSL

#extension GL_EXT_ray_query : require
#extension GL_EXT_nonuniform_qualifier : require

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#include "Common/Common.glsl"
#include "Hybrid/Hybrid.h"

layout(set = 2, binding = 6) readonly buffer Positions{ float positions[]; };
layout(set = 2, binding = 7) readonly buffer Tetrahedral{ Tetrahedron tetrahedral[]; };
layout(set = 2, binding = 8) readonly buffer Coefficients{ float coefficients[]; };

vec4 GetBaryCoord(vec3 position, uint tetIndex)
{
    const uint vertexIndex = tetrahedral[tetIndex].vertices[3];

    const vec3 position3 = vec3(
        positions[vertexIndex * 3 + 0],
        positions[vertexIndex * 3 + 1],
        positions[vertexIndex * 3 + 2]);

    const vec3 delta = position - position3;

    vec4 baryCoord = vec4(delta * mat3(tetrahedral[tetIndex].matrix), 0.0);
    baryCoord.w = 1.0 - baryCoord.x - baryCoord.y - baryCoord.z;

    return baryCoord;
}

uint FindMostNegative(vec4 baryCoord)
{
    uint index = 4;
    float value = -EPSILON;

    for (uint i = 0; i < 4; ++i)
    {
        if (baryCoord[i] < value)
        {
            index = i;
            value = baryCoord[i];
        }
    }

    return index;
}

vec3 SampleLightVolume(vec3 position, vec3 N)
{
    uint tetIndex = 0;
    vec4 baryCoord;

    while (true)
    {
        baryCoord = GetBaryCoord(position, tetIndex);

        const uint index = FindMostNegative(baryCoord);

        if (index < 4)
        {   
            tetIndex = tetrahedral[tetIndex].neighbors[index];
        }
        else
        {
            break;
        }
    }

    vec3 tetCoeffs[4][9];
    for (uint i = 0; i < 4; ++i)
    {
        const uint vertexIndex = tetrahedral[tetIndex].vertices[i];

        for (uint j = 0; j < 9; ++j)
        {
            tetCoeffs[i][j].r = coefficients[vertexIndex * 9 * 3 + j * 3 + 0];
            tetCoeffs[i][j].g = coefficients[vertexIndex * 9 * 3 + j * 3 + 1];
            tetCoeffs[i][j].b = coefficients[vertexIndex * 9 * 3 + j * 3 + 2];
        }
    }
    
    vec3 coeffs[9];
    for (uint i = 0; i < 9; ++i)
    {
        coeffs[i] = BaryLerp(tetCoeffs[0][i], tetCoeffs[1][i], tetCoeffs[2][i], tetCoeffs[3][i], baryCoord);
    }

    const float c1 = 0.429043;
    const float c2 = 0.511664;
    const float c3 = 0.743125;
    const float c4 = 0.886227;
    const float c5 = 0.247708;

    const vec3 result = c1 * coeffs[8].xyz * (Pow2(N.x) - Pow2(N.y)) 
        + c3 * coeffs[6].xyz * Pow2(N.z) + c4 * coeffs[0].xyz - c5 * coeffs[6].xyz
        + 2.0 * c1 * (coeffs[4].xyz * N.x * N.y + coeffs[7].xyz * N.x * N.z + coeffs[5].xyz * N.y * N.z)
        + 2.0 * c2 * (coeffs[3].xyz * N.x + coeffs[1].xyz * N.y + coeffs[2].xyz * N.z);

    return result;
}

#endif