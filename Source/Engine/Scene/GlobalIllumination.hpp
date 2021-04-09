#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class ScenePT;
class Environment;
struct AABBox;

struct SphericalHarmonicsGrid
{
    std::vector<glm::vec3> positions;
    Texture probeTexture;
};

class GlobalIllumination
{
public:
    GlobalIllumination() = default;

    SphericalHarmonicsGrid Generate(ScenePT* scene, Environment* environment, const AABBox& bbox) const;
};
