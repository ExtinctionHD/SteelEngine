#include "Engine/Render/RenderOptions.hpp"

#include "Engine/ConsoleVariable.hpp"

namespace RenderOptions
{
    bool reverseDepth = true;
    static CVarBool reverseDepthCVar("r.ReverseDepth", reverseDepth);

    bool forceForward = true;
    static CVarBool forceForwardCVar("r.ForceForward", forceForward);

    bool rayTracingAllowed = true;
    static CVarBool rayTracingAllowedCVar("r.RayTracingAllowed", rayTracingAllowed);

    bool pathTracingAllowed = true;
    static CVarBool pathTracingAllowedCVar("r.PathTracingAllowed", pathTracingAllowed);

    namespace Atmosphere
    {
        int32_t transmittanceLutExtent = 256;
        static CVarInt transmittanceLutExtentCVar(
                "r.atmo.transmittanceLutExtent", transmittanceLutExtent);

        int32_t multiScatteringLutExtent = 256;
        static CVarInt multiScatteringLutExtentCVar(
                "r.atmo.multiScatteringLutExtent", multiScatteringLutExtent);

        int32_t arialLutExtent = 256;
        static CVarInt arialLutExtentCVar(
                "r.atmo.arialLutExtent", arialLutExtent);

        int32_t arialLutDepth = 64;
        static CVarInt arialLutDepthCVar(
                "r.atmo.arialLutDepth", arialLutExtent);

        int32_t skyLutExtent = 128;
        static CVarInt skyLutExtentCVar(
                "r.atmo.skyLutExtent", skyLutExtent);
    }
}
