#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE FRAGMENT_STAGE
#pragma shader_stage(fragment)

#include "Hybrid/Skybox.layout"

void main() 
{
    outColor = vec4(texture(environmentMap, inPos).rgb, 1.0);
}
