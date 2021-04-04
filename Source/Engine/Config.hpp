#pragma once

#include "Engine/Window.hpp"
#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Systems/CameraSystem.hpp"
#include "Engine/EngineHelpers.hpp"

namespace Config
{
    enum class PathTracingMode
    {
        eRayTracing,
        eRayQueries
    };

    constexpr const char* kEngineName = "SteelEngine";

    constexpr vk::Extent2D kExtent(1280, 720);

    constexpr Window::Mode kWindowMode = Window::Mode::eWindowed;

    constexpr bool kVSyncEnabled = false;

    const Filepath kShadersDirectory("~/Shaders/");

    const Filepath kDefaultScenePath("~/Assets/Scenes/DamagedHelmet/DamagedHelmet.gltf");
    const Filepath kDefaultEnvironmentPath("~/Assets/Environments/SunnyHills.hdr");

    constexpr bool kUseDefaultAssets = true;

    constexpr bool kStaticCamera = false;

    constexpr PathTracingMode kPathTracingMode = PathTracingMode::eRayTracing;

    constexpr float kPointLightRadius = 0.05f;

    constexpr bool kReverseDepth = true;

    namespace DefaultCamera
    {
        constexpr Camera::Description kDescription{
            .position = Direction::kBackward * 5.0f,
            .target = Vector3::kZero,
            .up = Direction::kUp,
            .xFov = glm::radians(90.0f),
            .aspectRatio = 16.0f / 9.0f,
            .zNear = 0.01f,
            .zFar = 1000.0f
        };

        constexpr CameraSystem::Parameters kSystemParameters{
            .sensitivity = 1.0f,
            .baseSpeed = 2.0f,
            .speedMultiplier = 4.0f
        };

        const CameraSystem::MovementKeyBindings kMovementKeyBindings{
            { CameraSystem::MovementAxis::eForward, { Key::eW, Key::eS } },
            { CameraSystem::MovementAxis::eLeft, { Key::eA, Key::eD } },
            { CameraSystem::MovementAxis::eUp, { Key::eSpace, Key::eLeftControl } },
        };

        const CameraSystem::SpeedKeyBindings kSpeedKeyBindings{
            Key::e1, Key::e2, Key::e3, Key::e4, Key::e5
        };
    }
}
