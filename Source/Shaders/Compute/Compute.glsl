#ifndef COMPUTE_GLSL
#define COMPUTE_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#include "Common/Constants.glsl"

vec3 GetCubeDirection(uint faceIndex, vec2 uv)
{
    const vec2 xy = uv * 2.0 - 1.0;

    const vec3 normal = CUBE_FACES_N[faceIndex];
    const vec3 tangent = CUBE_FACES_T[faceIndex];
    const vec3 binormal = CUBE_FACES_B[faceIndex];
    
    return normalize(normal + xy.x * tangent + xy.y * binormal);
}

#endif