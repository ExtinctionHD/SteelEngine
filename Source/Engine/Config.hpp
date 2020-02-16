#pragma once

#include "Engine/Window.hpp"
#include "Engine/Filesystem.hpp"

namespace Config
{
    constexpr const char *kEngineName = "SteelEngine";

    constexpr vk::Extent2D kExtent(1920, 1080);

    constexpr eWindowMode kMode = eWindowMode::kWindowed;

    constexpr bool kVSyncEnabled = false;

    const Filepath kShadersDirectory("~/Shaders/");
}
