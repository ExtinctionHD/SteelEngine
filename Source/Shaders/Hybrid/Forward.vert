#version 460

#define SHADER_STAGE vertex
#pragma shader_stage(vertex)

#define DEPTH_ONLY 0
#define NORMAL_MAPPING 0

layout(push_constant) uniform PushConstants{
    mat4 transform;
};

layout(set = 0, binding = 0) uniform cameraUBO{ mat4 viewProj; };

layout(location = 0) in vec3 inPosition;
#if !DEPTH_ONLY
    layout(location = 1) in vec3 inNormal;
    layout(location = 2) in vec3 inTangent;
    layout(location = 3) in vec2 inTexCoord;

    layout(location = 0) out vec3 outPosition;
    layout(location = 1) out vec3 outNormal;
    layout(location = 2) out vec2 outTexCoord;
    #if NORMAL_MAPPING
        layout(location = 3) out vec3 outTangent;
    #endif
#endif

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
    const vec4 worldPosition = transform * vec4(inPosition, 1.0);

    #if !DEPTH_ONLY
        // TODO: move to CPU
        const mat4 normalTransform = transpose(inverse(transform));

        outPosition = worldPosition.xyz;
        outNormal = normalize(vec3(normalTransform * vec4(inNormal, 0.0)));
        outTexCoord = inTexCoord;
        
        #if NORMAL_MAPPING
            outTangent = normalize(vec3(normalTransform * vec4(inTangent, 0.0)));
        #endif
    #endif

    gl_Position = viewProj * worldPosition;
}