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

vec3 ToSrgb(vec3 linear)
{
    const vec3 higher = vec3(1.055f) * pow(linear, vec3(1.0f / 2.4f)) - vec3(0.055f);
    const vec3 lower = linear * vec3(12.92f);

    return mix(higher, lower, lessThan(linear, vec3(0.0031308f)));
}

vec3 ToLinear(vec3 srgb)
{
    const vec3 higher = pow((srgb + vec3(0.055f)) / vec3(1.055f), vec3(2.4f));
    const vec3 lower = srgb / vec3(12.92f);

    return mix(higher, lower, lessThan(srgb, vec3(0.04045f)));
}

float Luminance(vec3 color)
{
	return dot(color, vec3(0.2126f, 0.7152f, 0.0722f));
}

float Pow2(float x)
{
    return x * x;
}

#endif