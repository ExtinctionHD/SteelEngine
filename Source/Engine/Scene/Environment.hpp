#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Shaders/Common/Common.h"

class Filepath;

struct EnvironmentComponent
{
    static constexpr auto in_place_delete = true;

    Texture cubemapTexture;
    Texture irradianceTexture;
    Texture reflectionTexture;
    gpu::DirectLight directLight;
};

namespace EnvironmentHelpers
{
    EnvironmentComponent LoadEnvironment(const Filepath& panoramaPath);
}