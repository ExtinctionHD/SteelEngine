#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class Filepath;

struct EnvironmentComponent
{
    static constexpr auto in_place_delete = true;

    Texture cubemapTexture;
    Texture irradianceTexture;
    Texture reflectionTexture;
};

namespace EnvironmentHelpers
{
    EnvironmentComponent LoadEnvironment(const Filepath& panoramaPath);
}
