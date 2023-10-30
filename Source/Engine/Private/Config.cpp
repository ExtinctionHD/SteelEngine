#include "Engine/Config.hpp"
#include "Engine/Filesystem/Filesystem.hpp"

#include <glaze/glaze.hpp>

template <>
struct glz::meta<vk::Extent2D>
{
   using T = vk::Extent2D;
   static constexpr auto value = object("width", &T::width, "height", &T::height);
};

template <>
struct glz::meta<Config::Engine>
{
   using T = Config::Engine;
   static constexpr auto value = object(
   "vulkanValidationEnabled", &T::vulkanValidationEnabled,
   "startUpWindowMode", &T::startUpWindowMode,
   "defaultWindowExtent", &T::defaultWindowExtent,
   "defaultScenePath", &T::defaultScenePath,
   "defaultPanoramaPath", &T::defaultPanoramaPath,
   "vSyncEnabled", &T::vSyncEnabled,
   "rayTracingEnabled", &T::rayTracingEnabled,
   "pathTracingEnabled", &T::pathTracingEnabled,
   "forceForward", &T::forceForward
   );
};

template <>
struct glz::meta<Config::App>
{
   using T = Config::App;
   static constexpr auto value = object("autoplayAnims", &T::autoplayAnims, "animPlaySpeeds", &T::animPlaySpeeds);
};


Config::Engine Config::engine;
Config::Camera Config::camera;
Config::App Config::app;

void Config::Initialize()
{
    const Filepath engineConfigJsonPath(Config::engine.kEngineConfigDirectory);
    const std::string engineConfigJson = Filesystem::ReadFile(engineConfigJsonPath);

    auto error = glz::read_json(Config::engine, engineConfigJson);
    if (error != glz::error_code::none)
    {
        Assert(false);
    }

    const Filepath appConfigJsonPath(Config::engine.kAppConfigDirectory);
    const std::string appConfigJson = Filesystem::ReadFile(appConfigJsonPath);

    error = glz::read_json(Config::app, appConfigJson);
    if (error != glz::error_code::none)
    {
        Assert(false);
    }
}
