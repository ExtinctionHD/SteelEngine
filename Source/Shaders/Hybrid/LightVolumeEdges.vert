#version 460

#define SHADER_STAGE vertex
#pragma shader_stage(vertex)

layout(set = 0, binding = 0) uniform Camera{ mat4 viewProj; };

layout(location = 0) in vec3 inPosition;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
    gl_Position = viewProj * vec4(inPosition, 1.0);
}