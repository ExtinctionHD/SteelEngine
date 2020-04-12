#version 460
#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE miss
#pragma shader_stage(miss)

layout(location = 1) rayPayloadInNV float shadow;

void main()
{
    shadow = 0.0f;
}