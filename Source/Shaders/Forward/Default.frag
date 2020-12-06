#version 460
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#include "Common/Common.h"
#include "Common/Common.glsl"

layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;
layout(set = 1, binding = 1) uniform sampler2D roughnessMetallicTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;
layout(set = 1, binding = 3) uniform sampler2D occlusionTexture;
layout(set = 1, binding = 4) uniform sampler2D emissionTexture;

layout(set = 1, binding = 5) uniform Material{ MaterialFactors materialFactors; };

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    const vec3 baseColor = ToLinear(texture(baseColorTexture, inTexCoord).rgb);
    const vec2 roughnessMetallic = texture(roughnessMetallicTexture, inTexCoord).rg;
    const vec3 normalSample = texture(normalTexture, inTexCoord).xyz * 2.0 - 1.0;
    const float occlusion = texture(occlusionTexture, inTexCoord).r;
    const vec3 emission = ToLinear(texture(emissionTexture, inTexCoord).rgb);

    const vec3 N = GetTBN(inNormal, inTangent) * normalSample;

    const vec3 resultColor = ToneMapping(baseColor);
    outColor = vec4(resultColor, 1.0);
}