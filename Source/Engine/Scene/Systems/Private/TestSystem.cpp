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
            if (Timer::GetGlobalSeconds() > 8.0f && scene.testChild)
            {
                scene.AddScene(std::move(*scene.testChild), entity);

                scene.testChild = nullptr;
            }
        }
    }
}
