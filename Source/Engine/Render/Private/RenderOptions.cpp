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
}
