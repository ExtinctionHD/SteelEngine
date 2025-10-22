#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE FRAGMENT_STAGE
#pragma shader_stage(fragment)

#include "Common/Common.glsl"
#include "Common/Constants.glsl"

#include "Hybrid/Skybox.layout"

// TODO fix artifacts
void main() 
{

    const vec3 dir = normalize(mix(
        mix(frame.cam.topLeftDir, frame.cam.topRightDir, inUV.x),
        mix(frame.cam.bottomLeftDir, frame.cam.bottomRightDir, inUV.x),
        inUV.y));

    const float phi = atan(dir.z, dir.x);
    const float u = phi / (2 * PI);
    
    const float theta = asin(dir.y);
    const float v = 0.5 + 0.5 * sign(theta) * sqrt(abs(theta) / (PI / 2));

    const vec3 skyColor = texture(skyLut, vec2(u, v)).rgb;

    outColor = vec4(skyColor, 1.0);
}
