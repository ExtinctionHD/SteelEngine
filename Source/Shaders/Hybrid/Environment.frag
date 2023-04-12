#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE FRAGMENT_STAGE
#pragma shader_stage(fragment)

#include "Common/Common.glsl"

#include "Hybrid/Environment.layout"

void main() 
{
    const vec3 environmentSample = texture(environmentMap, normalize(inTexCoord)).rgb;

    outColor = vec4(ToneMapping(environmentSample), 1.0);
}