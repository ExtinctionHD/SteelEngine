#version 460

#define SHADER_STAGE vertex
#pragma shader_stage(vertex)

#include "Common/Common.h"

#define DEPTH_ONLY 0
#define NORMAL_MAPPING 1

layout(push_constant) uniform pushConstants{
    PushData push;
};

layout(set = 0, binding = 0) uniform dynamicUBO{ DynamicData dynamic; };

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec2 outTexCoord;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
    const vec4 worldPosition = push.transform * vec4(inPosition, 1.0);

    outPosition = worldPosition.xyz;
    outNormal = normalize(vec3(push.transform * vec4(inNormal, 0.0)));
    outTexCoord = inTexCoord;
    outTangent = normalize(vec3(push.transform * vec4(inTangent, 0.0)));

    gl_Position = dynamic.viewProj * worldPosition;
}