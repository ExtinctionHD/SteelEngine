#ifndef PATH_TRACING_GLSL
#define PATH_TRACING_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE raygen
#pragma shader_stage(raygen)
#extension GL_NV_ray_tracing : require
void main() {}
#endif

#define MAX_BOUNCE_COUNT 4

struct Payload
{
    float hitT;
    vec3 normal;
    vec3 tangent;
    vec2 texCoord;
    uint matId;
};

#endif