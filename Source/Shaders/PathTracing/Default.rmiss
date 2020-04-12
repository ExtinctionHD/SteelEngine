#version 460
#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE miss
#pragma shader_stage(miss)

#include "PathTracing/PathTracing.h"
#include "PathTracing/PathTracing.glsl"

layout(location = 0) rayPayloadInNV vec3 outColor;

void main()
{
    outColor = CLEAR_COLOR;
}