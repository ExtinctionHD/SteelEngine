#ifndef COMMON_GLSL
#define COMMON_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

mat3 GetTBN(vec3 N, vec3 T)
{
    T = normalize(T - dot(T, N) * N);
    const vec3 B = cross(N, T);

    return mat3(T, B, N);
}

vec3 TangentToWorld(vec3 v, mat3 TBN)
{
    return TBN * v;
}

vec3 WorldToTangent(vec3 v, mat3 TBN)
{
    return v * TBN;
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

#endif