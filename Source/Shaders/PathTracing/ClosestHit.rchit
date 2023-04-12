#version 460

#extension GL_GOOGLE_include_directive : require

#include "Common/Stages.h"
#define SHADER_STAGE CLOSEST_STAGE
#pragma shader_stage(closest)

#include "Common/Common.h"
#include "Common/Common.glsl"
#include "PathTracing/PathTracing.glsl"

#include "PathTracing/PathTracing.layout"

uvec3 GetIndices(uint instanceId, uint primitiveId)
{
    return indexBuffers[nonuniformEXT(instanceId)].indices[primitiveId];
}

vec3 GetNormal(uint instanceId, uint i)
{
    return normalBuffers[nonuniformEXT(instanceId)].normals[i];
}

vec3 GetTangent(uint instanceId, uint i)
{
    return tangentBuffers[nonuniformEXT(instanceId)].tangents[i];
}

vec2 GetTexCoord(uint instanceId, uint i)
{
    return texCoordBuffers[nonuniformEXT(instanceId)].texCoords[i];
}

void main()
{
    const uint instanceId = gl_InstanceCustomIndexEXT & 0x0000FFFF;
    const uint materialId = gl_InstanceCustomIndexEXT >> 16;

    const uvec3 indices = GetIndices(instanceId, gl_PrimitiveID);

    vec3 normals[3];
    vec3 tangents[3];
    vec2 texCoords[3];

    for (uint i = 0; i < 3; ++i)
    {
        normals[i] = GetNormal(instanceId, indices[i]);
        tangents[i] = GetTangent(instanceId, indices[i]);
        texCoords[i] = GetTexCoord(instanceId, indices[i]);
    }

    const vec3 baryCoord = vec3(1.0 - hitCoord.x - hitCoord.y, hitCoord.x, hitCoord.y);

    const vec3 normal = BaryLerp(normals[0].xyz, normals[1].xyz, normals[2].xyz, baryCoord);
    const vec3 tangent = BaryLerp(tangents[0].xyz, tangents[1].xyz, tangents[2].xyz, baryCoord);
    const vec2 texCoord = BaryLerp(texCoords[0], texCoords[1], texCoords[2], baryCoord);

    // TODO: move to CPU
    const mat3 normalTransform = transpose(inverse(mat3(gl_ObjectToWorldEXT)));

    payload.hitT = gl_HitTEXT;
    payload.normal = normalize(normalTransform * normal);
    payload.tangent = normalize(normalTransform * tangent);
    payload.texCoord = texCoord;
    payload.matId = materialId;

    if (gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT)
    {
        payload.normal = -payload.normal;
    } 
}
