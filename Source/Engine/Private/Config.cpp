#include "Engine/Config.hpp"
#include "Engine/Filesystem/Filesystem.hpp"

#include <glaze/core/macros.hpp>
#include <glaze/glaze.hpp>

// clang-format off
GLZ_META(vk::Extent2D, width, height);

GLZ_META(Config::Engine,
    vulkanValidationEnabled,
    startUpWindowMode,
    defaultWindowExtent,
    defaultScenePath,
    defaultPanoramaPath,
    vSyncEnabled,
    rayTracingEnabled,
    pathTracingEnabled,
    forceForward
);

GLZ_META(Config::App, autoplayAnims, animPlaySpeeds);
// clang-format on

Config::Engine Config::engine;
Config::Camera Config::camera;
Config::App Config::app;

void Config::Initialize()
{
    const Filepath engineConfigJsonPath(Config::engine.kEngineConfigDirectory);
    const std::string engineConfigJson = Filesystem::ReadFile(engineConfigJsonPath);

    auto error = glz::read_json(Config::engine, engineConfigJson);
    Assert(error == glz::error_code::none);

    const Filepath appConfigJsonPath(Config::engine.kAppConfigDirectory);
    const std::string appConfigJson = Filesystem::ReadFile(appConfigJsonPath);

    error = glz::read_json(Config::app, appConfigJson);
    Assert(error == glz::error_code::none);
}
