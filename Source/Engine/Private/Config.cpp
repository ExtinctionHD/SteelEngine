#include "Engine/Config.hpp"
#include "Engine/Filesystem/Filesystem.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <json.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

REFLECT(vk::Extent2D, width, height);

REFLECT(
Config::Engine,
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

REFLECT(
Config::App,
autoplayAnims,
animPlaySpeeds
);


Config::Engine Config::engine;
Config::Camera Config::camera;
Config::App Config::app;

// Reference code:
// create a json string representing the person
// std::string personJson = json::serialize(person);
// prettify and print the json string to stdout
// json::Prettifier prettifier;
// std::cout << prettifier.prettify(personJson) << std::endl;

void Config::Initialize()
{
    const Filepath engineConfigJsonPath(Config::engine.kEngineConfigDirectory);
    const std::string engineConfigJson = Filesystem::ReadFile(engineConfigJsonPath);
    Config::engine = json::deserialize<Config::Engine>(engineConfigJson);

    const Filepath appConfigJsonPath(Config::engine.kAppConfigDirectory);
    const std::string appConfigJson = Filesystem::ReadFile(appConfigJsonPath);
    Config::app = json::deserialize<Config::App>(appConfigJson);
}
