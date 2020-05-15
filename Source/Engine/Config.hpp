#pragma once

#include "Engine/Window.hpp"
#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/CameraSystem.hpp"

namespace Config
{
    constexpr const char *kEngineName = "SteelEngine";

    constexpr vk::Extent2D kExtent(1920, 1080);

    constexpr WindowMode kWindowMode = WindowMode::eWindowed;

    constexpr bool kVSyncEnabled = false;

    const Filepath kShadersDirectory("~/Shaders/");

    const Filepath kDefaultScenePath = Filepath("~/Assets/Scenes/Helmets/Helmets.gltf");

    constexpr CameraSystem::Parameters kCameraSystemParameters{ 1.0f, 2.0f, 4.0f };

    const CameraSystem::MovementKeyBindings kCameraMovementKeyBindings{
        { CameraSystem::MovementAxis::eForward, { Key::eW, Key::eS } },
        { CameraSystem::MovementAxis::eLeft, { Key::eA, Key::eD } },
        { CameraSystem::MovementAxis::eUp, { Key::eSpace, Key::eLeftControl } },
    };

    const CameraSystem::SpeedKeyBindings kCameraSpeedKeyBindings{
        Key::e1, Key::e2, Key::e3, Key::e4, Key::e5
    };
}
