#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants{
    mat4 transform;
};

layout(set = 0, binding = 0) uniform Camera{ mat4 viewProj; };

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inTexCoord;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
    gl_Position = viewProj * transform * vec4(inPosition, 1.0);
}