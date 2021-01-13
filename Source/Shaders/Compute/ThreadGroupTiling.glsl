#ifndef THREAD_GROUP_TILING_GLSL
#define THREAD_GROUP_TILING_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE compute
#pragma shader_stage(compute)
void main() {}
#endif

#include "Common/Constants.glsl"

#define OPTIMAL_TILE_WIDTH 8
#define GlobalTiledID ThreadGroupTiling(gl_NumWorkGroups.xy, gl_WorkGroupSize.xy, OPTIMAL_TILE_WIDTH, gl_LocalInvocationID.xy, gl_WorkGroupID.xy)

// https://developer.nvidia.com/blog/optimizing-compute-shaders-for-l2-locality-using-thread-group-id-swizzling/

// Divide the 2D-Dispatch_Grid into tiles of dimension [N, DipatchGridDim.y]
// “CTA” (Cooperative Thread Array) == Thread Group in DirectX terminology
uvec2 ThreadGroupTiling(
    const uvec2 dispatchGridDim, // Arguments of the Dispatch call (typically from a ConstantBuffer)
    const uvec2 ctaDim,         // Already known in HLSL, eg:[numthreads(8, 8, 1)] -> uvec2(8, 8)
    const uint maxTileWidth,    // User parameter (N). Recommended values: 8, 16 or 32.
    const uvec2 groupThreadID,  // SV_GroupThreadID
    const uvec2 groupId)        // SV_GroupID
{
    // A perfect tile is one with dimensions = [maxTileWidth, dispatchGridDim.y]
    const uint Number_of_CTAs_in_a_perfect_tile = maxTileWidth * dispatchGridDim.y;

    // Possible number of perfect tiles
    const uint Number_of_perfect_tiles = dispatchGridDim.x / maxTileWidth;

    // Total number of CTAs present in the perfect tiles
    const uint Total_CTAs_in_all_perfect_tiles = Number_of_perfect_tiles * maxTileWidth * dispatchGridDim.y;
    const uint vThreadGroupIDFlattened = dispatchGridDim.x * groupId.y + groupId.x;

    // Tile_ID_of_current_CTA : current CTA to TILE-ID mapping.
    const uint Tile_ID_of_current_CTA = vThreadGroupIDFlattened / Number_of_CTAs_in_a_perfect_tile;
    const uint Local_CTA_ID_within_current_tile = vThreadGroupIDFlattened % Number_of_CTAs_in_a_perfect_tile;
    
    uint Local_CTA_ID_y_within_current_tile;
    uint Local_CTA_ID_x_within_current_tile;

    if (Total_CTAs_in_all_perfect_tiles <= vThreadGroupIDFlattened)
    {
        // Path taken only if the last tile has imperfect dimensions and CTAs from the last tile are launched. 
        uint X_dimension_of_last_tile = dispatchGridDim.x % maxTileWidth;
        Local_CTA_ID_y_within_current_tile = Local_CTA_ID_within_current_tile / X_dimension_of_last_tile;
        Local_CTA_ID_x_within_current_tile = Local_CTA_ID_within_current_tile % X_dimension_of_last_tile;
    }
    else
    {
        Local_CTA_ID_y_within_current_tile = Local_CTA_ID_within_current_tile / maxTileWidth;
        Local_CTA_ID_x_within_current_tile = Local_CTA_ID_within_current_tile % maxTileWidth;
    }

    const uint Swizzled_vThreadGroupIDFlattened =
        Tile_ID_of_current_CTA * maxTileWidth +
        Local_CTA_ID_y_within_current_tile * dispatchGridDim.x +
        Local_CTA_ID_x_within_current_tile;

    uvec2 SwizzledvThreadGroupID;
    SwizzledvThreadGroupID.y = Swizzled_vThreadGroupIDFlattened / dispatchGridDim.x;
    SwizzledvThreadGroupID.x = Swizzled_vThreadGroupIDFlattened % dispatchGridDim.x;

    uvec2 SwizzledvThreadID;
    SwizzledvThreadID.x = ctaDim.x * SwizzledvThreadGroupID.x + groupThreadID.x;
    SwizzledvThreadID.y = ctaDim.y * SwizzledvThreadGroupID.y + groupThreadID.y;

    return SwizzledvThreadID.xy;
}

#endif