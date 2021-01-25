#ifndef RAY_QUERY_GLSL
#define RAY_QUERY_GLSL

#extension GL_EXT_ray_query : require
#extension GL_EXT_nonuniform_qualifier : require

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#include "Common/Common.glsl"
#include "RayTracing/RayTracing.h"

#define RAY_MIN_T 0.001
#define RAY_MAX_T 1000.0

layout(constant_id = 0) const uint MATERIAL_COUNT = 256;

layout(set = 2, binding = 0) uniform accelerationStructureEXT tlas;

layout(set = 2, binding = 1) uniform materialsBuffer{ MaterialRT materials[MATERIAL_COUNT]; };
layout(set = 2, binding = 2) uniform sampler2D textures[];

layout(set = 2, binding = 3) readonly buffer IndicesData{ uint indices[]; } indicesData[];
layout(set = 2, binding = 4) readonly buffer TexCoordsData{ vec2 texCoords[]; } texCoordsData[];

uvec3 GetIndices(uint instanceId, uint primitiveId)
{
    return uvec3(indicesData[nonuniformEXT(instanceId)].indices[primitiveId * 3 + 0],
                 indicesData[nonuniformEXT(instanceId)].indices[primitiveId * 3 + 1],
                 indicesData[nonuniformEXT(instanceId)].indices[primitiveId * 3 + 2]);
}

vec2 GetTexCoord(uint instanceId, uint i)
{
    return texCoordsData[nonuniformEXT(instanceId)].texCoords[i];
}

float TraceRay(vec3 origin, vec3 direction)
{
    rayQueryEXT rayQuery;

    const uint rayFlags = gl_RayFlagsTerminateOnFirstHitEXT;

    rayQueryInitializeEXT(rayQuery, tlas, rayFlags, 0xFF,
            origin, RAY_MIN_T, direction, RAY_MAX_T);

    while(rayQueryProceedEXT(rayQuery))
    {
        if (rayQueryGetIntersectionTypeEXT(rayQuery, false) == gl_RayQueryCandidateIntersectionTriangleEXT)
        {
            const uint customIndex = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false);
            const uint primitiveId = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
            const vec2 hitCoord = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);

            const uint instanceId = customIndex & 0x0000FFFF;
            const uint materialId = customIndex >> 16;

            const uvec3 indices = GetIndices(instanceId, primitiveId);

            const vec2 texCoord0 = GetTexCoord(instanceId, indices[0]);
            const vec2 texCoord1 = GetTexCoord(instanceId, indices[1]);
            const vec2 texCoord2 = GetTexCoord(instanceId, indices[2]);
            
            const vec3 baryCoord = vec3(1.0 - hitCoord.x - hitCoord.y, hitCoord.x, hitCoord.y);

            const vec2 texCoord = BaryLerp(texCoord0, texCoord1, texCoord2, baryCoord);

            const MaterialRT mat = materials[materialId];

            float alpha = mat.baseColorFactor.a;
            if (mat.baseColorTexture >= 0)
            {
                alpha *= texture(textures[nonuniformEXT(mat.baseColorTexture)], texCoord).a;
            }

            if (alpha >= mat.alphaCutoff)
            {
                rayQueryConfirmIntersectionEXT(rayQuery);
            }
        }
    }

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT)
    {
        return rayQueryGetIntersectionTEXT(rayQuery, true);
    }

    return -1.0;
}

bool IsMiss(float t)
{
    return t < 0.0;
}

#endif