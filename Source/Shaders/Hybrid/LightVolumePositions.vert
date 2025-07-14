#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE VERTEX_STAGE
#pragma shader_stage(vertex)

#include "Hybrid/LightVolumePositions.layout"

void main() 
{
    outIndex = gl_InstanceIndex;
    outNormal = normalize(inPos);

    gl_Position = frame.cam.viewProj * vec4(inPos + inOffset.xyz, 1.0);
}