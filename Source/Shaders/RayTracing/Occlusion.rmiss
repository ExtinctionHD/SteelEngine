#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE miss
#pragma shader_stage(miss)

#include "RayTracing/RayTracing.glsl"

layout(location = 1) rayPayloadInEXT Occlusion occlusion;

void main()
{
    occlusion.hitT = -1.0;
}