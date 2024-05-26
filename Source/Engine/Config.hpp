#pragma once

#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Scene/Systems/CameraSystem.hpp"
#include "Engine/EngineHelpers.hpp"

namespace Config
{
    constexpr const char* kEngineName = "SteelEngine";

    const std::vector<Filepath> kEngineLogos{
        Filepath("~/Assets/Logos/SteelEngineLogo_ExtraLarge.png"),
        Filepath("~/Assets/Logos/SteelEngineLogo_Large.png"),
        Filepath("~/Assets/Logos/SteelEngineLogo_Medium.png"),
        Filepath("~/Assets/Logos/SteelEngineLogo_Small.png")
    };

    namespace DefaultCamera
    {
        constexpr CameraLocation kLocation{
            .position = Direction::kBackward * 5.0f,
            .direction = Direction::kForward,
            .up = Direction::kUp
        };

        constexpr CameraProjection kProjection{
            .yFov = glm::radians(60.0f),
            .width = 16.0f,
            .height = 9.0f,
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

        constexpr MouseButton kControlMouseButton = MouseButton::eRight;
    }
}
