#version 460
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#include "Common/Common.glsl"

layout(set = 1, binding = 0) uniform samplerCube environmentMap;

layout(location = 0) in vec3 inTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    const vec3 environmentSample = texture(environmentMap, normalize(inTexCoord)).rgb;

    outColor = vec4(ToneMapping(environmentSample), 1.0);
}