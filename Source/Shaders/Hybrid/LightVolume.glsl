#ifndef LIGHT_VOLUME_GLSL
#define LIGHT_VOLUME_GLSL

#extension GL_EXT_ray_query : require
#extension GL_EXT_nonuniform_qualifier : require

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#include "Common/Common.glsl"

layout(set = 2, binding = 6) readonly buffer LightVolumePositions{ float lightVolumePositions[]; };
layout(set = 2, binding = 7) readonly buffer LightVolumeTetrahedral{ float lightVolumeTetrahedral[]; };
layout(set = 2, binding = 8) readonly buffer LightVolumeCoefficients{ float lightVolumeCoefficients[]; };

vec3 SampleLightVolume(vec3 position, vec3 N)
{
    return vec3(0.0);
}

#endif