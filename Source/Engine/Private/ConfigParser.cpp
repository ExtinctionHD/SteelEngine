#include "Engine/ConfigParser.hpp"
#include "Engine/Engine.hpp"
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

REFLECT(
EngineConfig,
VulkanValidationEnabled,
StartUpWindowMode,
WindowWidth,
WindowHeight,
DefaultScenePath,
DefaultPanoramaPath,
VSyncEnabled,
RayTracingEnabled,
PathTracingEnabled,
ForceForward
);

REFLECT(
AppConfig,
AutoplayAnims,
AnimPlaySpeeds
);

namespace ConfigParser
{
// Reference code:
// create a json string representing the person
// std::string personJson = json::serialize(person);
// prettify and print the json string to stdout
// json::Prettifier prettifier;
// std::cout << prettifier.prettify(personJson) << std::endl;

void ApplyIniConfigs()
{
    const Filepath engineConfigJsonPath(Config::kEngineConfigDirectory);
    const std::string engineConfigJson = Filesystem::ReadFile(engineConfigJsonPath);
    const EngineConfig engineConfig = json::deserialize<EngineConfig>(engineConfigJson);

    Engine::Config = engineConfig;

    const Filepath appConfigJsonPath(Config::kAppConfigDirectory);
    const std::string appConfigJson = Filesystem::ReadFile(appConfigJsonPath);
    const AppConfig appConfig = json::deserialize<AppConfig>(appConfigJson);

    Engine::AppConfig = appConfig;
}
}
