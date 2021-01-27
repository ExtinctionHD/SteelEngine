#ifndef COMMON_GLSL
#define COMMON_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#include "Common/Constants.glsl"

vec2 BaryLerp(vec2 a, vec2 b, vec2 c, vec3 barycentrics)
{
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec3 BaryLerp(vec3 a, vec3 b, vec3 c, vec3 barycentrics)
{
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec4 BaryLerp(vec4 a, vec4 b, vec4 c, vec3 barycentrics)
{
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

mat3 GetTBN(vec3 N, vec3 T)
{
    T = normalize(T - dot(T, N) * N);
    const vec3 B = cross(N, T);

    return mat3(T, B, N);
}

mat3 GetTBN(vec3 N)
{
    mat3 TBN;

	vec3 T = cross(N, UNIT_Y);
	T = mix(cross(N, UNIT_X), T, step(EPSILON, dot(T, T)));
	T = normalize(T);

	const vec3 B = normalize(cross(N, T));

    return mat3(T, B, N);
}

/*
mat3 GetTBN(vec3 N)
{
    const float s = sign(N.z);
    const float a = -1.0 / (s + N.z);
    const float b = N.x * N.y * a;
    
    vec3 T = vec3(1 + s * a * N.x * N.x, s * b, -s * N.x);
    vec3 B = vec3(b,  s + a * N.y * N.y, -N.y);

    return mat3(T, B, N);
}
*/

vec3 TangentToWorld(vec3 v, mat3 TBN)
{
    return TBN * v;
}

vec3 WorldToTangent(vec3 v, mat3 TBN)
{
    return v * TBN;
}

float CosThetaWorld(vec3 N, vec3 v)
{
    return max(dot(N, v), 0.0);
}

float CosThetaTangent(vec3 v)
{
    return max(v.z, 0.0);
}

vec3 ToSrgb(vec3 linear)
{
    const vec3 higher = vec3(1.055) * pow(linear, vec3(1 / 2.4)) - vec3(0.055);
    const vec3 lower = linear * vec3(12.92);

    return mix(higher, lower, lessThan(linear, vec3(0.0031308)));
}

vec3 ToLinear(vec3 srgb)
{
    const vec3 higher = pow((srgb + vec3(0.055)) / vec3(1.055), vec3(2.4));
    const vec3 lower = srgb / vec3(12.92);

    return mix(higher, lower, lessThan(srgb, vec3(0.04045)));
}

float Luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

bool IsBlack(vec3 color)
{
    return dot(color, color) < EPSILON;
}

bool Emits(vec3 emitting)
{
    return dot(emitting, emitting) > EPSILON;
}

vec3 ToneMapping(vec3 linear)
{
    linear = max(vec3(0), linear - vec3(0.004));
    const vec3 srgb = (linear * (6.2 * linear + 0.5)) / (linear * (6.2 * linear + 1.7) + 0.06);
    return srgb;
}

vec3 UnchartedToneMapping(vec3 linear)
{
    const float A = 0.22;
    const float B = 0.30;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.01;
    const float F = 0.30;
    const float WP = 11.2;
    linear = ((linear * (A * linear + C * B) + D * E) / (linear * (A * linear + B) + D * F)) - E / F;
    linear /= ((WP * (A * WP + C * B) + D * E) / (WP * (A * WP + B) + D * F)) - E / F;
    return linear;
}

float MaxComponent(vec3 v)
{
    return max(v.x, max(v.y, v.z));
}

float Saturate(float p)
{
    return clamp(p, 0.0, 1.0);
}

float Pow5(float p)
{
    return p * p * p * p * p;
}

float Rcp(float p)
{
    return p == 0.0 ? 1e10 : 1.0 / p;
}

vec3 FaceForward(vec3 N, vec3 V)
{
    return faceforward(N, -V, N);
}

bool IsMiss(float t)
{
    return t < 0.0;
}

#endif