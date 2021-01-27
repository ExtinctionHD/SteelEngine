#ifndef RAY_TRACING_GLSL
#define RAY_TRACING_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#define RAY_MIN_T 0.001
#define RAY_MAX_T 1000.0

struct Ray
{
    vec3 origin;
    vec3 direction;
    float TMin;
    float TMax;
};

bool IsMiss(float t)
{
    return t < 0.0;
}

#endif