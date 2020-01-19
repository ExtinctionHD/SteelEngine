#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec3 outPos;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{	
    outPos = inPos;
    gl_Position = vec4(inPos, 1.0f);
}