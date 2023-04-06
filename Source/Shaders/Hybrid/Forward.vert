#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE VERTEX_STAGE
#pragma shader_stage(vertex)

#define DEPTH_ONLY 0
#define NORMAL_MAPPING 0

#include "Hybrid/Forward.layout"

void main() 
{
    const vec4 worldPosition = transform * vec4(inPosition, 1.0);

    #if !DEPTH_ONLY
        // TODO: move to CPU
        const mat4 normalTransform = transpose(inverse(transform));

        outPosition = worldPosition.xyz;
        outNormal = normalize(vec3(normalTransform * vec4(inNormal, 0.0)));
        outTexCoord = inTexCoord;
        
        #if NORMAL_MAPPING
            outTangent = normalize(vec3(normalTransform * vec4(inTangent, 0.0)));
        #endif
    #endif

    gl_Position = viewProj * worldPosition;
}