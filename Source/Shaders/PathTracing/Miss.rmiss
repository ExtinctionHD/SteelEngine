#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE miss
#pragma shader_stage(miss)

#include "Common/RayTracing.h"
#include "PathTracing/PathTracing.glsl"

layout(set = 2, binding = 2) uniform samplerCube envMap;

layout(location = 0) rayPayloadInEXT Payload rayPayload;

void main()
{
    rayPayload.hitT = -1.0;
    rayPayload.normal = texture(envMap, gl_WorldRayDirectionEXT).rgb;
}