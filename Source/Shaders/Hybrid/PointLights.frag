#version 460
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#include "Common/Common.glsl"

layout(location = 0) in vec3 inColor;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(ToneMapping(inColor), 1.0);
}