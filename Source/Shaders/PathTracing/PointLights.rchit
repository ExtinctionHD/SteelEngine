#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE closest
#pragma shader_stage(closest)

#include "Common/Common.h"
#include "PathTracing/PathTracing.glsl"

#define POINT_LIGHT_COUNT 4

layout(set = 5, binding = 0) uniform colorsBuffer{
    vec4 colors[POINT_LIGHT_COUNT];
};

layout(location = 0) rayPayloadInEXT MaterialPayload payload;

void main()
{
    payload.hitT = gl_HitTEXT;
    payload.normal = colors[gl_InstanceID].rgb;
}
