#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE VERTEX_STAGE
#pragma shader_stage(vertex)

#include "Hybrid/LightVolumePositions.layout"

void main() 
{
    outIndex = gl_InstanceIndex;
    outNormal = normalize(inPosition);

    gl_Position = viewProj * vec4(inPosition + inOffset.xyz, 1.0);
}