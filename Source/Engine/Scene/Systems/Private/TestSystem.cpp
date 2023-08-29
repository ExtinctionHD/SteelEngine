#include "Engine/Scene/Systems/TestSystem.hpp"

#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/Components/Components.hpp"

#include "Utils/TimeHelpers.hpp"

void TestSystem::Process(Scene& scene, float)
{
    entt::entity spawn = entt::null;
    entt::entity helmet = entt::null;

    for (auto&& [entity, nc] : scene.view<NameComponent>().each())
    {
        if (nc.name == "damaged_helmet_spawn")
        {
            spawn = entity;
        }
        if (nc.name == "damaged_helmet")
        {
            helmet = entity;
        }
    }

    if (spawn != entt::null && helmet != entt::null)
    {
        if (static bool instantiated = false; !instantiated && Timer::GetGlobalSeconds() > 8.0f)
        {
            scene.InstantiateScene(helmet, scene.AddEntity(spawn, {}));

            instantiated = true;
        }

        if (static bool erased = false; !erased && Timer::GetGlobalSeconds() > 12.0f)
        {
            helmetScene = scene.EraseScene(helmet);

            erased = true;
        }

        if (helmetScene && Timer::GetGlobalSeconds() > 14.0f)
        {
            scene.InsertScene(std::move(*helmetScene), helmet);
            scene.InstantiateScene(helmet, scene.AddEntity(spawn, {}));

            helmetScene.reset();
        }

        if (static bool removed = false; !removed && Timer::GetGlobalSeconds() > 18.0f)
        {
            scene.RemoveEntity(helmet);

            removed = true;
        }
    }
}
