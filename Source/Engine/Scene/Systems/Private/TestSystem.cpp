#include "Engine/Scene/Systems/TestSystem.hpp"

#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/Components/Components.hpp"

#include "Utils/TimeHelpers.hpp"

void TestSystem::Process(Scene& scene, float)
{
    for (auto&& [entity, nc] : scene.view<NameComponent>().each())
    {
        if (nc.name == "damaged_helmet_spawn")
        {
            static bool instantiated = false;

            if (!instantiated && Timer::GetGlobalSeconds() > 8.0f)
            {
                scene.InstantiateScene(scene.FindEntity("damaged_helmet"), scene.AddEntity(entity, {}));

                instantiated = true;
            }
        }
    }
}
