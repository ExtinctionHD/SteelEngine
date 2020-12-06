#version 460
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#include "Common/Common.glsl"

layout(set = 1, binding = 0) uniform samplerCube environment;

layout(location = 0) in vec3 inTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    const vec3 environmentSample = texture(environment, normalize(inTexCoord)).rgb;

    const vec3 resultColor = ToneMapping(environmentSample);
    outColor = vec4(resultColor, 1.0);
}