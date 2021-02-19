#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE miss
#pragma shader_stage(miss)

#include "Common/RayTracing.h"
#include "PathTracing/PathTracing.glsl"

layout(location = 0) rayPayloadInEXT MaterialPayload payload;
layout(location = 1) rayPayloadInEXT ColorPayload pointLightPayload;

void main()
{
    payload.hitT = -1.0;
    pointLightPayload.hitT = -1.0;
}