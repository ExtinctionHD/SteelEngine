#ifndef UTILS_GLSL
#define UTILS_GLSL

#ifndef SHADER_STAGE
#pragma shader_stage(vertex)
void main() {}
#endif

mat3 GetTBN(vec3 N, vec3 T)
{
    T = normalize(T - dot(T, N) * N);
    const vec3 B = cross(N, T);

    return mat3(T, B, N);
}

#endif