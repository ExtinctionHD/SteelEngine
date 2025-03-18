#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class Filepath;

struct EnvironmentComponent
{
    Texture cubemapTexture;
    Texture irradianceTexture;
    Texture reflectionTexture;
};

namespace EnvironmentHelpers
{
    EnvironmentComponent LoadEnvironment(const Filepath& panoramaPath);
}
