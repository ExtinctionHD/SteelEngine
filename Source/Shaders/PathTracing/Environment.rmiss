#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE miss
#pragma shader_stage(miss)

layout(location = 1) rayPayloadInEXT float envHit;

void main()
{
    envHit = 1;
}