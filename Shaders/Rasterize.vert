#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Camera{
    mat4 viewProj;
};

layout(set = 1, binding = 0) uniform Transform{
    mat4 transform;
};

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoord;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{	
    outNormal = inNormal;
    outTexCoord = inTexCoord;

    gl_Position = viewProj * transform * vec4(inPos, 1.0f);
}