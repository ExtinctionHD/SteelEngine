#pragma once

#include "Engine/Scene/Systems/CameraSystem.hpp"
#include "Engine/EngineHelpers.hpp"
#include "Engine/Window.hpp"

class Config
{
public:
    struct Engine
    {
        bool vulkanValidationEnabled = true;

        Window::Mode startUpWindowMode = Window::Mode::eWindowed;
        vk::Extent2D defaultWindowExtent = vk::Extent2D{1280, 720};

        std::string defaultScenePath = "~/Assets/Scenes/CornellBox/CornellBox.gltf"; // "~/Assets/Scenes/Sponza/Sponza.gltf"
        std::string defaultPanoramaPath = "~/Assets/Environments/SunnyHills.hdr"; // "~/Assets/Environments/DuskHills.hdr"

        bool vSyncEnabled = false;

        bool rayTracingEnabled = false;

        bool pathTracingEnabled = false;

        bool forceForward = false;

        static constexpr const char* kEngineName = "SteelEngine";

        static constexpr bool kUseDefaultAssets = true;

        static constexpr bool kReverseDepth = true;

        const std::string kEngineLogoExtraLarge{"~/Assets/Logos/SteelEngineLogo_ExtraLarge.png"};
        const std::string kEngineLogoLarge{"~/Assets/Logos/SteelEngineLogo_Large.png"};
        const std::string kEngineLogoMedium{"~/Assets/Logos/SteelEngineLogo_Medium.png"};
        const std::string kEngineLogoSmall{"~/Assets/Logos/SteelEngineLogo_Small.png"};

        const std::string kShadersDirectory{"~/Shaders/"};
        const std::string kEngineConfigDirectory{"~/Config/EngineConfig.ini"};
        const std::string kAppConfigDirectory{"~/Config/AppConfig.ini"};
    };

    struct Camera
    {
        static constexpr bool kStaticCamera = false;
        static constexpr MouseButton kControlMouseButton = MouseButton::eRight;

        static constexpr CameraLocation kLocation{
            .position = Direction::kBackward * 5.0f,
            .direction = Direction::kForward,
            .up = Direction::kUp
        };

        static constexpr CameraProjection kProjection{
            .yFov = glm::radians(60.0f),
            .width = 16.0f,
            .height = 9.0f,
            .zNear = 0.01f,
            .zFar = 1000.0f
        };

        static constexpr CameraSystem::Parameters kSystemParameters{
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
    };

    struct App
    {
        std::set<std::string> autoplayAnims = {};
        std::map<std::string, float> animPlaySpeeds = {};
    };

    static void Initialize();

    static Engine engine;
    static Camera camera;
    static App app;
};

