#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define SHADER_STAGE closest
#pragma shader_stage(closest)

#include "RayTracing/RayTracing.glsl"
#include "Common/Common.glsl"

layout(set = 6, binding = 0) readonly buffer IndicesData{ uint indices[]; } indicesData[];
layout(set = 7, binding = 0) readonly buffer NormalsData{ float normals[]; } normalsData[];
layout(set = 8, binding = 0) readonly buffer TangentsData{ float tangents[]; } tangentsData[];
layout(set = 9, binding = 0) readonly buffer TexCoordsData{ vec2 texCoords[]; } texCoordsData[];

layout(location = 0) rayPayloadInEXT Payload rayPayload;

hitAttributeEXT vec2 hitCoord;

uvec3 GetIndices(uint instanceId, uint primitiveId)
{
    return uvec3(indicesData[nonuniformEXT(instanceId)].indices[primitiveId * 3 + 0],
                 indicesData[nonuniformEXT(instanceId)].indices[primitiveId * 3 + 1],
                 indicesData[nonuniformEXT(instanceId)].indices[primitiveId * 3 + 2]);
}

vec3 GetNormal(uint instanceId, uint i)
{
    return vec3(normalsData[nonuniformEXT(instanceId)].normals[i * 3 + 0],
                normalsData[nonuniformEXT(instanceId)].normals[i * 3 + 1],
                normalsData[nonuniformEXT(instanceId)].normals[i * 3 + 2]);
}

vec3 GetTangent(uint instanceId, uint i)
{
    return vec3(tangentsData[nonuniformEXT(instanceId)].tangents[i * 3 + 0],
                tangentsData[nonuniformEXT(instanceId)].tangents[i * 3 + 1],
                tangentsData[nonuniformEXT(instanceId)].tangents[i * 3 + 2]);
}

vec2 GetTexCoord(uint instanceId, uint i)
{
    return texCoordsData[nonuniformEXT(instanceId)].texCoords[i];
}

void main()
{
    const uint instanceId = gl_InstanceCustomIndexEXT & 0x0000FFFF;
    const uint materialId = gl_InstanceCustomIndexEXT >> 16;

    const uvec3 indices = GetIndices(instanceId, gl_PrimitiveID);

    const vec3 normal0 = GetNormal(instanceId, indices[0]);
    const vec3 normal1 = GetNormal(instanceId, indices[1]);
    const vec3 normal2 = GetNormal(instanceId, indices[2]);

    const vec3 tangent0 = GetTangent(instanceId, indices[0]);
    const vec3 tangent1 = GetTangent(instanceId, indices[1]);
    const vec3 tangent2 = GetTangent(instanceId, indices[2]);

    const vec2 texCoord0 = GetTexCoord(instanceId, indices[0]);
    const vec2 texCoord1 = GetTexCoord(instanceId, indices[1]);
    const vec2 texCoord2 = GetTexCoord(instanceId, indices[2]);
    
    const vec3 baryCoord = vec3(1.0 - hitCoord.x - hitCoord.y, hitCoord.x, hitCoord.y);
    
    const vec3 normal = BaryLerp(normal0, normal1, normal2, baryCoord);
    const vec3 tangent = BaryLerp(tangent0, tangent1, tangent2, baryCoord);
    const vec2 texCoord = BaryLerp(texCoord0, texCoord1, texCoord2, baryCoord);
    
    rayPayload.hitT = gl_HitTEXT;
    rayPayload.normal = normalize(gl_ObjectToWorldEXT * vec4(normal, 0.0));
    rayPayload.tangent = normalize(gl_ObjectToWorldEXT * vec4(tangent, 0.0));
    rayPayload.texCoord = texCoord;
    rayPayload.matId = materialId;
}
