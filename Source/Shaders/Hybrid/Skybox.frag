#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE FRAGMENT_STAGE
#pragma shader_stage(fragment)

#include "Common/Common.glsl"
#include "Common/Constants.glsl"
#include "Common/Atmosphere.glsl"

#include "Hybrid/Skybox.layout"

void main() 
{
    const vec3 dir = normalize(mix(
        mix(frame.cam.topLeftDir, frame.cam.topRightDir, inUV.x),
        mix(frame.cam.bottomLeftDir, frame.cam.bottomRightDir, inUV.x),
        inUV.y));

    const vec3 skyColor = texture(skyLut, GetSkyUV(dir)).rgb;

    outColor = vec4(skyColor, 1.0);
}
