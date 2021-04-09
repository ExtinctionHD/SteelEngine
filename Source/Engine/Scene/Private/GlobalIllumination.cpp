#include "Engine/Scene/GlobalIllumination.hpp"

#include "Engine/Render/ProbeRenderer.hpp"
#include "Utils/Helpers.hpp"

namespace Details
{
    static constexpr float kStep = 0.5f;

    static std::vector<glm::vec3> GeneratePositions(const AABBox& bbox)
    {
        const glm::uvec3 size = glm::uvec3((bbox.max - bbox.min) / kStep) + glm::uvec3(1);

        std::vector<glm::vec3> positions;
        positions.reserve(size.x * size.y * size.z);

        for (uint32_t i = 0; i < size.x; ++i)
        {
            for (uint32_t j = 0; j < size.y; ++j)
            {
                for (uint32_t k = 0; k < size.z; ++k)
                {
                    positions.push_back(bbox.min + glm::vec3(i, j, k) * kStep);
                }
            }
        }

        return positions;
    }
}

SphericalHarmonicsGrid GlobalIllumination::Generate(ScenePT* scene, Environment* environment, const AABBox& bbox) const
{
    std::unique_ptr<ProbeRenderer> probeRenderer = std::make_unique<ProbeRenderer>(scene, environment);

    const std::vector<glm::vec3> positions = Details::GeneratePositions(bbox);

    const glm::vec3 modernSponzaTestPosition(-0.5f, 2.5f, -0.28f);

    const Texture probeTexture = probeRenderer->CaptureProbe(modernSponzaTestPosition);

    return SphericalHarmonicsGrid{ positions, probeTexture };
}
