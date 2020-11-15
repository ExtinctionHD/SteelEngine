#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants{
    mat4 transform;
};

layout(set = 0, binding = 0) uniform Camera{ mat4 viewProj; };

layout(location = 0) out vec3 outTexCoord;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
    const vec3 position = vec3(
        ((gl_VertexIndex  & 0x4) == 0) ? -1.0 : 1.0,
        ((gl_VertexIndex  & 0x2) == 0) ? -1.0 : 1.0,
        ((gl_VertexIndex  & 0x1) == 0) ? -1.0 : 1.0
    );

    gl_Position = viewProj * transform * vec4(position, 1.0);
}