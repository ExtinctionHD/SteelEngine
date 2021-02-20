#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE closest
#pragma shader_stage(closest)

#include "Common/Common.h"
#include "PathTracing/PathTracing.glsl"

#define POINT_LIGHT_COUNT 4

layout(set = 4, binding = 2) uniform colorsBuffer{
    vec4 colors[POINT_LIGHT_COUNT];
};

layout(location = 1) rayPayloadInEXT ColorPayload pointLightPayload;

void main()
{
    pointLightPayload.hitT = gl_HitTEXT;
    pointLightPayload.color = colors[gl_InstanceID].rgb;
}
