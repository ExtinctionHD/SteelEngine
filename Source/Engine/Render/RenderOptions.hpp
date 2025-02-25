#pragma once

namespace RenderOptions
{
    extern bool reverseDepth;

    extern bool forceForward;

    extern bool rayTracingAllowed;

    extern bool pathTracingAllowed;

    namespace Atmosphere
    {
        extern int32_t transmittanceLutExtent;

        extern int32_t multiScatteringLutExtent;

        extern int32_t arialLutExtent;

        extern int32_t arialLutDepth;

        extern int32_t skyLutExtent;
    }
}
