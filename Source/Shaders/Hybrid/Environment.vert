#version 460

#define SHADER_STAGE vertex
#pragma shader_stage(vertex)

#define REVERSE_DEPTH 1

layout(set = 0, binding = 0) uniform Camera{ mat4 viewProj; };

layout(location = 0) out vec3 outTexCoord;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
    const vec3 position = vec3(
        ((gl_VertexIndex & 0x4) == 0) ? 1.0 : -1.0,
        ((gl_VertexIndex & 0x2) == 0) ? 1.0 : -1.0,
        ((gl_VertexIndex & 0x1) == 0) ? 1.0 : -1.0
    );

    outTexCoord = position;

    vec4 projectedPosition = viewProj * vec4(position, 1.0);

#if REVERSE_DEPTH
    projectedPosition.z = 0.0;
#else
    projectedPosition.z = projectedPosition.w;
#endif

    gl_Position = projectedPosition;
}