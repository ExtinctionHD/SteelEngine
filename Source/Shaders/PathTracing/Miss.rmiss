#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE MISS_STAGE
#pragma shader_stage(miss)

#include "PathTracing/PathTracing.layout"

void main()
{
    payload.hitT = -1.0;
}