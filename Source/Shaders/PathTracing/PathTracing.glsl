#ifndef PATH_TRACING_GLSL
#define PATH_TRACING_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE raygen
#pragma shader_stage(raygen)
#extension GL_NV_ray_tracing : require
void main() {}
#endif

#define MAX_DEPTH 4

struct Payload
{
    vec3 T;
    vec3 L;
    uint depth;
    uvec2 seed;
};

#endif