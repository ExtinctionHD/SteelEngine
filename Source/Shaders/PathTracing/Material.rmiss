#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE miss
#pragma shader_stage(miss)

#include "PathTracing/PathTracing.h"
#include "PathTracing/PathTracing.glsl"

layout(set = 2, binding = 3) uniform samplerCube envMap;

layout(location = 0) rayPayloadInEXT Payload raygen;

void main()
{
    if (raygen.depth == 0)
    {
        raygen.L = texture(envMap, gl_WorldRayDirectionEXT).rgb;
    }
    else
    {
        raygen.L = vec3(0);
    }
}