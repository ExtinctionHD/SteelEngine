#pragma once

#include "Engine/Window.hpp"
#include "Engine/Scene/Systems/CameraSystem.hpp"
#include "Engine/EngineHelpers.hpp"

// TODO implement config.ini file
namespace Config
{
    constexpr const char* kEngineName = "SteelEngine";

    constexpr vk::Extent2D kExtent(1280, 720);

    constexpr Window::Mode kWindowMode = Window::Mode::eWindowed;

    const std::string kShadersDirectory("~/Shaders/");

    const std::string kDefaultScenePath("~/Assets/Scenes/CornellBox/CornellBox.gltf");
    //const std::string kDefaultScenePath("~/Assets/Scenes/Sponza/Sponza.gltf");

    const std::string kDefaultPanoramaPath("~/Assets/Environments/SunnyHills.hdr");
    //const std::string kDefaultPanoramaPath("~/Assets/Environments/DuskHills.hdr");

    const std::vector<std::string> kEngineLogos{
        std::string("~/Assets/Logos/SteelEngineLogo_ExtraLarge.png"),
        std::string("~/Assets/Logos/SteelEngineLogo_Large.png"),
        std::string("~/Assets/Logos/SteelEngineLogo_Medium.png"),
        std::string("~/Assets/Logos/SteelEngineLogo_Small.png")
    };

    constexpr float kLightProbeRadius = 0.1f;

    constexpr bool kVSyncEnabled = false;

    constexpr bool kUseDefaultAssets = true;

    constexpr bool kStaticCamera = false;

    constexpr bool kRayTracingEnabled = false;

    constexpr bool kPathTracingEnabled = false;

    constexpr bool kGlobalIlluminationEnabled = false; // not working

    constexpr bool kReverseDepth = true;

    constexpr bool kForceForward = false;

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
