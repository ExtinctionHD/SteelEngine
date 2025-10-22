#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE VERTEX_STAGE
#pragma shader_stage(vertex)

#define REVERSE_DEPTH 1

#include "Common/Constants.glsl"

#include "Hybrid/Skybox.layout"

void main() 
{
    const vec2 pos[3] = {
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0),
    };

    const vec2 p = pos[gl_VertexIndex];
    
    outUV = p * 0.5 + 0.5;

    gl_Position = vec4(p, 0.0, 1.0);

#if REVERSE_DEPTH
    gl_Position.z = 0.0;
#else
    gl_Position.z = gl_Position.w;
#endif
}
