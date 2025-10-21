#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE VERTEX_STAGE
#pragma shader_stage(vertex)

// TODO implement global defines
#define REVERSE_DEPTH 1

#include "Common/Constants.glsl"

#include "Hybrid/Skybox.layout"

void main() 
{
    outPos = inPos;

    gl_Position = frame.cam.proj * mat4(mat3(frame.cam.view)) * vec4(inPos, 1.0);

#if REVERSE_DEPTH
    gl_Position.z = EPSILON;
#else
    gl_Position.z = 1.0 - EPSILON;
#endif
}
