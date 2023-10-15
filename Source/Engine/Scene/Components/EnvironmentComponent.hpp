#pragma once

#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class Filepath;

struct EnvironmentComponent
{
    static constexpr auto in_place_delete = true;

    CubeImage cubemapImage;
    CubeImage irradianceImage;
    CubeImage reflectionImage;
};

namespace EnvironmentHelpers
{
    EnvironmentComponent LoadEnvironment(const Filepath& panoramaPath);
}
