#pragma once

#include "Engine/Scene/Systems/CameraSystem.hpp"
#include "Engine/EngineHelpers.hpp"

struct EngineConfig
{
    bool VulkanValidationEnabled = true;

    std::string StartUpWindowMode = "windowed"; // "windowed" / "borderless" / "fullscreen"

    int WindowWidth = 1280;
    int WindowHeight = 720;

    std::string DefaultScenePath = "~/Assets/Scenes/CornellBox/CornellBox.gltf"; // "~/Assets/Scenes/Sponza/Sponza.gltf"
    std::string DefaultPanoramaPath = "~/Assets/Environments/SunnyHills.hdr"; // "~/Assets/Environments/DuskHills.hdr"

    bool VSyncEnabled = false;

    bool RayTracingEnabled = false;

    bool PathTracingEnabled = false;

    bool ForceForward = false;
};

struct AppConfig
{
    std::set<std::string> AutoplayAnims = {};
    std::map<std::string, float> AnimPlaySpeeds = {};
};

namespace Config
{
    constexpr bool kConfigOverrideFromIniFilesAllowed = true;
    constexpr bool kForceDisableVulkanValidationRelease = true;

    constexpr const char* kEngineName = "SteelEngine";

    const std::vector<std::string> kEngineLogos{
        std::string("~/Assets/Logos/SteelEngineLogo_ExtraLarge.png"),
        std::string("~/Assets/Logos/SteelEngineLogo_Large.png"),
        std::string("~/Assets/Logos/SteelEngineLogo_Medium.png"),
        std::string("~/Assets/Logos/SteelEngineLogo_Small.png")
    };

    const std::string kShadersDirectory("~/Shaders/");
    const std::string kEngineConfigDirectory("~/Config/EngineConfig.ini");
    const std::string kAppConfigDirectory("~/Config/AppConfig.ini");

    constexpr bool kUseDefaultAssets = true;

    constexpr bool kStaticCamera = false;

    constexpr bool kReverseDepth = true;

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
