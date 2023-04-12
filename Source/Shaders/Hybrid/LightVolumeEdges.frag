#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE FRAGMENT_STAGE
#pragma shader_stage(fragment)

#include "Hybrid/LightVolumeEdges.layout"

void main()
{
    outColor = vec4(0.8, 0.8, 0.8, 1.0);
}