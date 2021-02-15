#version 460

#define SHADER_STAGE vertex
#pragma shader_stage(vertex)

layout(set = 0, binding = 0) uniform Camera{ mat4 viewProj; };

layout(location = 0) in vec3 inPosition;

layout(location = 1) in vec4 inOffset;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec3 outColor;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
    outColor = inColor.rgb;

    gl_Position = viewProj * vec4(inPosition + inOffset.xyz, 1.0);
}