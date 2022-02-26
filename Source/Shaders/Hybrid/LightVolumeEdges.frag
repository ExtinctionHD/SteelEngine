#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#include "Common/Common.glsl"

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(0.8, 0.8, 0.8, 1.0);
}