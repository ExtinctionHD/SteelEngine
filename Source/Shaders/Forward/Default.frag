#version 460
#extension GL_GOOGLE_include_directive : require

#include "Common/Common.h"

layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;
layout(set = 1, binding = 1) uniform sampler2D roughnessMetallicTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;
layout(set = 1, binding = 3) uniform sampler2D occlusionTexture;
layout(set = 1, binding = 4) uniform sampler2D emissionTexture;

layout(set = 1, binding = 5) uniform Material{ MaterialFactors materialFactors; };

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = texture(baseColorTexture, inTexCoord);
}