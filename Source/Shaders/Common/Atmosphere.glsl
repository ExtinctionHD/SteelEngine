#ifndef ATMOSPHERE_GLSL
#define ATMOSPHERE_GLSL

#ifndef SHADER_STAGE
    #define SHADER_STAGE vertex
    #pragma shader_stage(vertex)
    void main() {}
#endif

#include "Common/Common.h"
#include "Common/Constants.glsl"

#ifndef ATMOSPHERE
Atmosphere atmo;
#define ATMOSPHERE atmo
#endif

float GetRayleightDensity(float h)
{
    return exp(-h / ATMOSPHERE.rayleightDensityHeight);
}

float GetMieDensity(float h)
{
    return exp(-h / ATMOSPHERE.mieDensityHeight);
}

float GetOzoneDensity(float h)
{
    return max(0.0, 1.0 - 0.5 * abs(h - ATMOSPHERE.ozoneCenterHeight) / ATMOSPHERE.ozoneThickness);
}

vec3 GetSigmaT(float h)
{
    const vec3 rayleigh = ATMOSPHERE.rayleightScattering * GetRayleightDensity(h);
    const float mie = (ATMOSPHERE.mieScattering + ATMOSPHERE.mieAbsorption) * GetMieDensity(h);
    const vec3 ozone = ATMOSPHERE.ozoneAbsorption * GetOzoneDensity(h);

    return rayleigh + mie + ozone;
}

void GetSigmaST(float h, out vec3 sigmaS, out vec3 sigmaT)
{
    const vec3 rayleigh = ATMOSPHERE.rayleightScattering * GetRayleightDensity(h);

    const float mieDensity = GetMieDensity(h);
    const float mieS = ATMOSPHERE.mieScattering * mieDensity;
    const float mieT = (ATMOSPHERE.mieScattering + ATMOSPHERE.mieAbsorption) * mieDensity;

    const vec3 ozone = ATMOSPHERE.ozoneAbsorption * GetOzoneDensity(h);

    sigmaS = rayleigh + mieS;
    sigmaT = rayleigh + mieT + ozone;
}

vec2 GetTransmittanceUV(float h, float theta) // TODO STEEL-1 sinTheta
{
    const float u = h / (ATMOSPHERE.atmosphereRadius - ATMOSPHERE.planetRadius);
    const float v = 0.5 + 0.5 * sin(theta);
    return vec2(u, v);
}

vec2 GetMultiScatteringUV(float h, float theta) // TODO STEEL-1 sinTheta
{
    const float u = h / (ATMOSPHERE.atmosphereRadius - ATMOSPHERE.planetRadius);
    const float v = 0.5 + 0.5 * sin(theta);
    return vec2(u, v);
}

vec3 EvaluatePhaseFunction(float h, float u)
{
    vec3 sRayleigh = ATMOSPHERE.rayleightScattering * exp(-h / ATMOSPHERE.rayleightDensityHeight);
    float sMie = ATMOSPHERE.mieScattering * exp(-h / ATMOSPHERE.mieDensityHeight);
    vec3 s = sRayleigh + sMie;

    float g = ATMOSPHERE.mieScatteringAsymmetry;
    float g2 = g * g;
    float u2 = u * u;
    float pRayleigh = 3.0 / (16.0 * PI) * (1.0 + u2);

    float m = 1.0 + g2 - 2.0 * g * u;
    float pMie = 3.0 / (8.0 * PI) * (1.0 - g2) * (1.0 + u2) / ((2.0 + g2) * m * sqrt(m));

    vec3 result;
    result.x = s.x > 0.0 ? (pRayleigh * sRayleigh.x + pMie * sMie) / s.x : 0.0;
    result.y = s.y > 0.0 ? (pRayleigh * sRayleigh.y + pMie * sMie) / s.y : 0.0;
    result.z = s.z > 0.0 ? (pRayleigh * sRayleigh.z + pMie * sMie) / s.z : 0.0;

    return result;
}

#endif