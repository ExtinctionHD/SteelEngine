#version 460

#define SHADER_STAGE vertex
#pragma shader_stage(vertex)

layout(set = 0, binding = 0) uniform Camera{ mat4 viewProj; };

layout(location = 0) in vec3 inPosition;

layout(location = 1) in vec3 inOffset;
layout(location = 2) in uvec3 inCoord;

layout(location = 0) out vec3 outCoord;
layout(location = 1) out vec3 outNormal;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
    outCoord = inCoord;
    outNormal = normalize(inPosition);

    gl_Position = viewProj * vec4(inPosition + inOffset.xyz, 1.0);
}