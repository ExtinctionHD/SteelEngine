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
#include "Hybrid/Hybrid.glsl"

layout(set = 2, binding = 4) readonly buffer Positions{ float positions[]; };
layout(set = 2, binding = 5) readonly buffer Tetrahedral{ Tetrahedron tetrahedral[]; };
layout(set = 2, binding = 6) readonly buffer Coefficients{ float coefficients[]; };

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

int FindMostNegative(vec4 baryCoord)
{
    int index = -1;
    float value = -EPSILON;

    for (int i = 0; i < VERTEX_COUNT; ++i)
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
    int tetIndex = 0;
    int coordIndex;
    vec4 baryCoord;

    do
    {
        baryCoord = GetBaryCoord(position, tetIndex);
        coordIndex = FindMostNegative(baryCoord);

        if (coordIndex >= 0)
        {   
            tetIndex = tetrahedral[tetIndex].neighbors[coordIndex];

            if (tetIndex < 0)
            {
                return vec3(0.0);
            }
        }
    }
    while (coordIndex >= 0);

    vec3 tetCoeffs[VERTEX_COUNT][COEFFICIENT_COUNT];
    for (uint i = 0; i < VERTEX_COUNT; ++i)
    {
        const uint vertexIndex = tetrahedral[tetIndex].vertices[i];

        for (uint j = 0; j < COEFFICIENT_COUNT; ++j)
        {
            const uint offset = vertexIndex * COEFFICIENT_COUNT * 3 + j * 3;

            tetCoeffs[i][j].r = coefficients[offset + 0];
            tetCoeffs[i][j].g = coefficients[offset + 1];
            tetCoeffs[i][j].b = coefficients[offset + 2];
        }
    }
    
    vec3 coeffs[COEFFICIENT_COUNT];
    for (uint i = 0; i < COEFFICIENT_COUNT; ++i)
    {
        coeffs[i] = BaryLerp(tetCoeffs[0][i], tetCoeffs[1][i], tetCoeffs[2][i], tetCoeffs[3][i], baryCoord);
    }

    return CalculateIrradiance(coeffs, N);
}

#endif