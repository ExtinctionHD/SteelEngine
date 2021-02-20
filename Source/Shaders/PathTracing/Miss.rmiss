#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE miss
#pragma shader_stage(miss)

#define PAYLOAD_LOCATION 0

layout(location = PAYLOAD_LOCATION) rayPayloadInEXT float hitT;

void main()
{
    hitT = -1.0;
}