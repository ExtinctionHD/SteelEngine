#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{	
    outTexCoord = inTexCoord;
    gl_Position = vec4(inPos, 1.0f);
}