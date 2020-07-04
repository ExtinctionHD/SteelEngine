#ifndef MONTE_CARLO_GLSL
#define MONTE_CARLO_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#include "Common/Constants.glsl"

uint ReverseBits32(uint bits) 
{
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
    bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
    bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
    bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
    return bits;
}

vec2 Hammersley(uint i, uint N, uvec2 random)
{
    const float E1 = fract(float(i) / N + float(random.x & 0xFFFF) / (1 << 16));
    const float E2 = float(ReverseBits32(i) ^ random.y) * 2.3283064365386963e-10;
    return vec2(E1, E2);
}

vec3 CosineSampleHemisphere(vec2 E)
{
    const float phi = 2.0 * PI * E.x;
    const float cosTheta = sqrt(E.y);
    const float sinTheta = sqrt(1 - cosTheta * cosTheta);

    vec3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    return H;
}

float CosinePdfHemisphere(float cosTheta)
{
    return cosTheta * INVERSE_PI;
}

float PowerHeuristic(float pdfA, float pdfB)
{
    const float f = pdfA * pdfA;
    const float g = pdfB * pdfB;
    return f / (f + g);
}

#endif