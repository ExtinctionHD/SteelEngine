#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE VERTEX_STAGE
#pragma shader_stage(vertex)

#define REVERSE_DEPTH 1

#include "Hybrid/Environment.layout"

void main() 
{
    const vec3 position = vec3(
        ((gl_VertexIndex & 0x4) == 0) ? 1.0 : -1.0,
        ((gl_VertexIndex & 0x2) == 0) ? 1.0 : -1.0,
        ((gl_VertexIndex & 0x1) == 0) ? 1.0 : -1.0
    );

    outTexCoord = position;

    vec4 projectedPosition = viewProj * vec4(position, 1.0);

    #if REVERSE_DEPTH
        projectedPosition.z = 0.0;
    #else
        projectedPosition.z = projectedPosition.w;
    #endif

    gl_Position = projectedPosition;
}