#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE VERTEX_STAGE
#pragma shader_stage(vertex)

#include "Hybrid/LightVolumeEdges.layout"

void main() 
{
    gl_Position = viewProj * vec4(inPosition, 1.0);
}