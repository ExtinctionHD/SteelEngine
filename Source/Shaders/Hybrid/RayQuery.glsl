#ifndef RAY_QUERY_GLSL
#define RAY_QUERY_GLSL

#extension GL_EXT_ray_query : require

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#define RAY_MIN_T 0.001
#define RAY_MAX_T 1000.0

layout(set = 2, binding = 0) uniform accelerationStructureEXT tlas;

float TraceRay(vec3 origin, vec3 direction)
{
    rayQueryEXT rayQuery;

    const uint flags = gl_RayFlagsTerminateOnFirstHitEXT;

    rayQueryInitializeEXT(rayQuery, tlas, flags, 0xFF,
            origin, RAY_MIN_T, direction, RAY_MAX_T);

    while(rayQueryProceedEXT(rayQuery)) {}  

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