#version 460

#define SHADER_STAGE vertex
#pragma shader_stage(vertex)

layout(push_constant) uniform PushConstants{
    mat4 transform;
};

layout(set = 0, binding = 0) uniform Camera{ mat4 viewProj; };

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec2 outTexCoord;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
    const vec4 worldPosition = transform * vec4(inPosition, 1.0);

    outPosition = worldPosition.xyz;
    outNormal = vec3(transform * vec4(inNormal, 0.0));
    outTangent = vec3(transform * vec4(inTangent, 0.0));
    outTexCoord = inTexCoord;

    gl_Position = viewProj * worldPosition;
}