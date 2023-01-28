#ifndef RAY_QUERY_GLSL
#define RAY_QUERY_GLSL

#extension GL_EXT_ray_query : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : enable

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#include "Common/Common.h"
#include "Common/Common.glsl"
#include "Common/RayTracing.glsl"

layout(constant_id = 2) const uint MATERIAL_COUNT = 256;

layout(set = 4, binding = 0) uniform accelerationStructureEXT tlas;

layout(set = 4, binding = 1) uniform materialsBuffer{ Material materials[MATERIAL_COUNT]; };
layout(set = 4, binding = 2) uniform sampler2D textures[];

layout(set = 4, binding = 3, scalar) readonly buffer IndexBuffers{ uvec3 indices[]; } indexBuffers[];
layout(set = 4, binding = 4, scalar) readonly buffer TexCoordBuffers{ vec2 texCoords[]; } texCoordBuffers[];

uvec3 GetIndices(uint instanceId, uint primitiveId)
{
    return indexBuffers[nonuniformEXT(instanceId)].indices[primitiveId];
}

vec2 GetTexCoord(uint instanceId, uint i)
{
    return texCoordBuffers[nonuniformEXT(instanceId)].texCoords[i];
}

float TraceRay(Ray ray)
{
    rayQueryEXT rayQuery;

    const uint rayFlags = gl_RayFlagsTerminateOnFirstHitEXT;

    rayQueryInitializeEXT(rayQuery, tlas, rayFlags, 0xFF,
            ray.origin, ray.TMin, ray.direction, ray.TMax);

    while (rayQueryProceedEXT(rayQuery))
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

            const Material mat = materials[materialId];

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

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
    {
        return rayQueryGetIntersectionTEXT(rayQuery, true);
    }

    return -1.0;
}

#endif