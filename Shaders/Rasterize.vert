#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants
{
    layout(offset = 0) mat4 viewProj;
};

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
    gl_Position = viewProj * vec4(inPos, 1.0f);
}