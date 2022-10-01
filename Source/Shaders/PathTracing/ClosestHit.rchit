#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define SHADER_STAGE closest
#pragma shader_stage(closest)

#include "Common/Common.h"
#include "Common/Common.glsl"
#include "PathTracing/PathTracing.glsl"

layout(set = 2, binding = 5) readonly buffer IndicesData{ uint indices[]; } indicesData[];
layout(set = 2, binding = 6) readonly buffer VerticesData{ VertexRT vertices[]; } verticesData[];

layout(location = 0) rayPayloadInEXT MaterialPayload payload;

hitAttributeEXT vec2 hitCoord;

uvec3 GetIndices(uint instanceId, uint primitiveId)
{
    return uvec3(indicesData[nonuniformEXT(instanceId)].indices[primitiveId * 3 + 0],
                 indicesData[nonuniformEXT(instanceId)].indices[primitiveId * 3 + 1],
                 indicesData[nonuniformEXT(instanceId)].indices[primitiveId * 3 + 2]);
}

// VertexRT GetVertex(uint instanceId, uint i)
// {
//     VertexRT vertex;
//     vertex.normal.x = verticesData[nonuniformEXT(instanceId)].vertices[i * 8 + 0];
//     vertex.normal.y = verticesData[nonuniformEXT(instanceId)].vertices[i * 8 + 1];
//     vertex.normal.z = verticesData[nonuniformEXT(instanceId)].vertices[i * 8 + 2];
    
//     vertex.tangent.x = verticesData[nonuniformEXT(instanceId)].vertices[i * 8 + 3];
//     vertex.tangent.y = verticesData[nonuniformEXT(instanceId)].vertices[i * 8 + 4];
//     vertex.tangent.z = verticesData[nonuniformEXT(instanceId)].vertices[i * 8 + 5];
    
//     vertex.texCoord.x = verticesData[nonuniformEXT(instanceId)].vertices[i * 8 + 6];
//     vertex.texCoord.y = verticesData[nonuniformEXT(instanceId)].vertices[i * 8 + 7];

//     return vertex;
// }

VertexRT GetVertex(uint instanceId, uint i)
{
    return verticesData[nonuniformEXT(instanceId)].vertices[i];
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
        const VertexRT vertex = GetVertex(instanceId, indices[i]);

        normals[i] = vertex.normal.xyz;
        tangents[i] = vertex.tangent.xyz;

        texCoords[i].x = vertex.normal.w;
        texCoords[i].y = vertex.tangent.w;
    }

    const vec3 baryCoord = vec3(1.0 - hitCoord.x - hitCoord.y, hitCoord.x, hitCoord.y);

    const vec3 normal = BaryLerp(normals[0].xyz, normals[1].xyz, normals[2].xyz, baryCoord);
    const vec3 tangent = BaryLerp(tangents[0].xyz, tangents[1].xyz, tangents[2].xyz, baryCoord);
    const vec2 texCoord = BaryLerp(texCoords[0], texCoords[1], texCoords[2], baryCoord);

    payload.hitT = gl_HitTEXT;
    payload.normal = normalize(gl_ObjectToWorldEXT * vec4(normal, 0.0));
    payload.tangent = normalize(gl_ObjectToWorldEXT * vec4(tangent, 0.0));
    payload.texCoord = texCoord;
    payload.matId = materialId;

    if (gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT)
    {
        payload.normal = -payload.normal;
    } 
}
