#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_query : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : enable

#ifndef SHADER_STAGE
    #include "Common/Stages.h"
    #define SHADER_STAGE COMPUTE_STAGE
    #pragma shader_stage(compute)
    void main() {}
#endif

#include "Common/Common.h"
#include "Common/Common.glsl"
#include "Common/Debug.glsl"
#include "Common/PBR.glsl"
#include "Common/RayTracing.glsl"

#ifndef SHADER_LAYOUT
    #include "Hybrid/Lighting.layout"
#endif

#if RAY_TRACING_ENABLED
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
                    alpha *= texture(materialTextures[nonuniformEXT(mat.baseColorTexture)], texCoord).a;
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

#if LIGHT_VOLUME_ENABLED
    vec4 GetBaryCoord(vec3 position, uint tetIndex)
    {
        const uint vertexIndex = tetrahedral[tetIndex].vertices[3];

        const vec3 position3 = vec3(
            positions[vertexIndex * 3 + 0],
            positions[vertexIndex * 3 + 1],
            positions[vertexIndex * 3 + 2]);

        const vec3 delta = position - position3;

        vec4 baryCoord = vec4(delta * mat3(tetrahedral[tetIndex].matrix), 0.0);
        baryCoord.w = 1.0 - baryCoord.x - baryCoord.y - baryCoord.z;

        return baryCoord;
    }

    int FindMostNegative(vec4 baryCoord)
    {
        int index = -1;
        float value = 0.0;

        for (int i = 0; i < TET_VERTEX_COUNT; ++i)
        {
            if (baryCoord[i] < value)
            {
                index = i;
                value = baryCoord[i];
            }
        }

        return index;
    }

    vec3 SampleLightVolume(vec3 position, vec3 N)
    {
        int tetIndex = 0;
        int prevTetIndex = 0;

        vec4 baryCoord;
        int coordIndex;

        do
        {
            baryCoord = GetBaryCoord(position, tetIndex);
            coordIndex = FindMostNegative(baryCoord);

            if (coordIndex >= 0)
            {   
                const int nextTetIndex = tetrahedral[tetIndex].neighbors[coordIndex];

                if (prevTetIndex == nextTetIndex)
                {
                    break;
                }

                prevTetIndex = tetIndex;
                tetIndex = nextTetIndex;

                if (tetIndex < 0)
                {
                    return vec3(0.0);
                }
            }
        }
        while (coordIndex >= 0);

        vec3 tetCoeffs[TET_VERTEX_COUNT][SH_COEFFICIENT_COUNT];
        for (uint i = 0; i < TET_VERTEX_COUNT; ++i)
        {
            const uint vertexIndex = tetrahedral[tetIndex].vertices[i];

            for (uint j = 0; j < SH_COEFFICIENT_COUNT; ++j)
            {
                const uint offset = vertexIndex * SH_COEFFICIENT_COUNT * 3 + j * 3;

                tetCoeffs[i][j].r = coefficients[offset + 0];
                tetCoeffs[i][j].g = coefficients[offset + 1];
                tetCoeffs[i][j].b = coefficients[offset + 2];
            }
        }
        
        // TODO add normal based weighting

        vec3 coeffs[SH_COEFFICIENT_COUNT];
        for (uint i = 0; i < SH_COEFFICIENT_COUNT; ++i)
        {
            coeffs[i] = BaryLerp(tetCoeffs[0][i], tetCoeffs[1][i], tetCoeffs[2][i], tetCoeffs[3][i], baryCoord);
        }

        return ComputeIrradiance(coeffs, N);
    }
#endif

vec3 ComputeDirectLighting(vec3 position, vec3 N, vec3 V, float NoV, vec3 baseColor, vec3 F0, float roughness, float metallic)
{
#if DEBUG_VIEW_DIRECT_LIGHTING && LIGHT_COUNT > 0
    vec3 directLighting = vec3(0.0);
    for (uint i = 0; i < LIGHT_COUNT; ++i)
    {
        const Light light = lights[i];

        const float a = roughness * roughness;
        const float a2 = a * a;
        
        const vec3 direction = light.location.xyz - position * light.location.w;

        const float distance = Select(RAY_MAX_T, length(direction), light.location.w);
        const float attenuation = Select(1.0, Rcp(Pow2(distance)), light.location.w);

        const vec3 L = normalize(direction);
        const vec3 H = normalize(L + V);

        const float NoL = CosThetaWorld(N, L);
        const float NoH = CosThetaWorld(N, H);
        const float VoH = max(dot(V, H), 0.0);

        const float irradiance = attenuation * NoL * Luminance(light.color.rgb);

        if (irradiance > EPSILON)
        {
            const float D = D_GGX(a2, NoH);
            const vec3 F = F_Schlick(F0, VoH);
            const float Vis = Vis_Schlick(a, NoV, NoL);
            
            const vec3 kD = mix(vec3(1.0) - F, vec3(0.0), metallic);
            
            const vec3 diffuse = kD * Diffuse_Lambert(baseColor);
            const vec3 specular = D * F * Vis;

            Ray ray;
            ray.origin = position + N * BIAS;
            ray.direction = L;
            ray.TMin = RAY_MIN_T;
            ray.TMax = distance;
            
            #if RAY_TRACING_ENABLED
                const float shadow = IsMiss(TraceRay(ray)) ? 0.0 : 1.0;
            #else
                const float shadow = 0.0;
            #endif

            const vec3 lighting = NoL * light.color.rgb * (1.0 - shadow) * attenuation;

            directLighting += ComposeBRDF(diffuse, specular) * lighting;
        }
    }
    return directLighting;
#else
    return vec3(0.0);
#endif
}

vec3 ComputeIndirectLighting(vec3 position, vec3 N, vec3 V, float NoV, vec3 baseColor, vec3 F0, float roughness, float metallic, float occlusion)
{
#if DEBUG_VIEW_INDIRECT_LIGHTING
    #if LIGHT_VOLUME_ENABLED
        const vec3 irradiance = SampleLightVolume(position, N);
        const vec3 envIrradiance = texture(irradianceMap, N).rgb;
        const vec3 specularNorm = irradiance / envIrradiance;
    #else
        const vec3 irradiance = texture(irradianceMap, N).rgb;
        const vec3 specularNorm = vec3(1.0);
    #endif

    const vec3 kS = F_SchlickRoughness(F0, NoV, roughness);
    const vec3 kD = mix(vec3(1.0) - kS, vec3(0.0), metallic);

    const vec3 R = -reflect(V, N);
    const float lod = roughness * (textureQueryLevels(reflectionMap) - 1);
    const vec3 reflection = textureLod(reflectionMap, R, lod).rgb;

    const vec2 scaleOffset = texture(specularBRDF, vec2(NoV, roughness)).xy;
    
    const vec3 diffuse = kD * irradiance * baseColor;
    const vec3 specular = (F0 * scaleOffset.x + scaleOffset.y) * reflection;
    
    return ComposeBRDF(diffuse, specular * specularNorm) * occlusion;
#else
    return vec3(0.0);
#endif
}

#endif