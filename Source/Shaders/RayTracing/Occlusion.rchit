#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE closest
#pragma shader_stage(closest)

#include "RayTracing/RayTracing.glsl"

layout(location = 1) rayPayloadInEXT Occlusion occlusion;

void main()
{
    occlusion.hitT = gl_HitTEXT;
}