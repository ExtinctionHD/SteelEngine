#include "Engine/Engine.hpp"

#include "Engine/Config.hpp"
#include "Engine/Filesystem/Filesystem.hpp"
#include "Engine/Scene/SceneModel.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/ScenePT.hpp"
#include "Engine/Scene/Environment.hpp"
#include "Engine/Systems/CameraSystem.hpp"
#include "Engine/Systems/UIRenderSystem.hpp"
#include "Engine/Render/PathTracer.hpp"
#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/FrameLoop.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace Details
{
    static Filepath GetScenePath()
    {
        if constexpr (Config::kUseDefaultAssets)
        {
            return Config::kDefaultScenePath;
        }
        else
        {
            const DialogDescription dialogDescription{
                "Select Scene File", Filepath("~/"),
                { "glTF Files", "*.gltf" }
            };

            const std::optional<Filepath> scenePath = Filesystem::ShowOpenDialog(dialogDescription);

            return scenePath.value_or(Config::kDefaultScenePath);
        }
    }

    static Filepath GetEnvironmentPath()
    {
        if constexpr (Config::kUseDefaultAssets)
        {
            return Config::kDefaultEnvironmentPath;
        }
        else
        {
            const DialogDescription dialogDescription{
                "Select Environment File", Filepath("~/"),
                { "Image Files", "*.hdr *.png" }
            };

            const std::optional<Filepath> environmentPath = Filesystem::ShowOpenDialog(dialogDescription);

            return environmentPath.value_or(Config::kDefaultEnvironmentPath);
        }
    }

    static std::string GetCameraPositionText(const Camera& camera)
    {
        const Camera::Description& cameraDescription = camera.GetDescription();

        const glm::vec3 cameraPosition = cameraDescription.position;

        return Format("Camera position: %.2f %.2f %.2f", cameraPosition.x, cameraPosition.y, cameraPosition.z);
    }

    static std::string GetCameraDirectionText(const Camera& camera)
    {
        const Camera::Description& cameraDescription = camera.GetDescription();

        const glm::vec3 cameraDirection = glm::normalize(cameraDescription.target - cameraDescription.position);

        return Format("Camera direction: %.2f %.2f %.2f", cameraDirection.x, cameraDirection.y, cameraDirection.z);
    }

    static std::string GetLightDirectionText(const Environment& environment)
    {
        const glm::vec4& lightDirection = environment.GetDirectLight().direction;

        return Format("Light direction: %.2f %.2f %.2f", lightDirection.x, lightDirection.y, lightDirection.z);
    }

    static std::string GetLightColorText(const Environment& environment)
    {
        const glm::vec4& lightColor = environment.GetDirectLight().color;

        return Format("Light color: %.2f %.2f %.2f", lightColor.r, lightColor.g, lightColor.b);
    }
}

Timer Engine::timer;
Engine::State Engine::state;

std::unique_ptr<Window> Engine::window;
std::unique_ptr<FrameLoop> Engine::frameLoop;

std::unique_ptr<SceneModel> Engine::sceneModel;
std::unique_ptr<Environment> Engine::environment;

std::unique_ptr<Scene> Engine::scene;
std::unique_ptr<ScenePT> Engine::scenePT;
std::unique_ptr<Camera> Engine::camera;

std::unique_ptr<Renderer> Engine::renderer;
std::unique_ptr<PathTracer> Engine::pathTracer;

std::vector<std::unique_ptr<System>> Engine::systems;
std::map<EventType, std::vector<EventHandler>> Engine::eventMap;

void Engine::Create()
{
    window = std::make_unique<Window>(Config::kExtent, Config::kWindowMode);

    VulkanContext::Create(*window);
    RenderContext::Create();

    AddEventHandler<vk::Extent2D>(EventType::eResize, &Engine::HandleResizeEvent);
    AddEventHandler<KeyInput>(EventType::eKeyInput, &Engine::HandleKeyInputEvent);

    frameLoop = std::make_unique<FrameLoop>();
    sceneModel = std::make_unique<SceneModel>(Details::GetScenePath());
    environment = std::make_unique<Environment>(Details::GetEnvironmentPath());

    scene = sceneModel->CreateScene();
    scenePT = sceneModel->CreateScenePT();
    camera = sceneModel->CreateCamera();

    renderer = std::make_unique<Renderer>(scene.get(), scenePT.get(), camera.get(), environment.get());
    pathTracer = std::make_unique<PathTracer>(scenePT.get(), camera.get(), environment.get());

    AddSystem<CameraSystem>(camera.get());
    AddSystem<UIRenderSystem>(*window);

    GetSystem<UIRenderSystem>()->BindText([]() { return Details::GetCameraPositionText(*camera); });
    GetSystem<UIRenderSystem>()->BindText([]() { return Details::GetCameraDirectionText(*camera); });
    GetSystem<UIRenderSystem>()->BindText([]() { return Details::GetLightDirectionText(*environment); });
    GetSystem<UIRenderSystem>()->BindText([]() { return Details::GetLightColorText(*environment); });
}

void Engine::Run()
{
    while (!window->ShouldClose())
    {
        window->PollEvents();

        for (auto& system : systems)
        {
            system->Process(timer.GetDeltaSeconds());
        }

        if (state.drawingSuspended)
        {
            continue;
        }

        frameLoop->Draw([](vk::CommandBuffer commandBuffer, uint32_t imageIndex)
            {
                if (state.renderMode == RenderMode::ePathTracing)
                {
                    pathTracer->Render(commandBuffer, imageIndex);
                }
                else
                {
                    renderer->Render(commandBuffer, imageIndex);
                }

                GetSystem<UIRenderSystem>()->Render(commandBuffer, imageIndex);
            });
    }
}

void Engine::Destroy()
{
    VulkanContext::device->WaitIdle();

    systems.clear();

    renderer.reset();
    pathTracer.reset();
    camera.reset();
    scene.reset();
    scenePT.reset();
    environment.reset();
    sceneModel.reset();
    frameLoop.reset();
    window.reset();

    RenderContext::Destroy();

    VulkanContext::Destroy();
}

void Engine::TriggerEvent(EventType type)
{
    for (const auto& handler : eventMap[type])
    {
        handler(std::any());
    }
}

void Engine::AddEventHandler(EventType type, std::function<void()> handler)
{
    std::vector<EventHandler>& eventHandlers = eventMap[type];
    eventHandlers.emplace_back([handler](std::any)
        {
            handler();
        });
}

void Engine::HandleResizeEvent(const vk::Extent2D& extent)
{
    VulkanContext::device->WaitIdle();

    state.drawingSuspended = extent.width == 0 || extent.height == 0;

    if (!state.drawingSuspended)
    {
        const Swapchain::Description swapchainDescription{
            extent, Config::kVSyncEnabled
        };

        VulkanContext::swapchain->Recreate(swapchainDescription);
    }
}

void Engine::HandleKeyInputEvent(const KeyInput& keyInput)
{
    if (keyInput.action == KeyAction::ePress)
    {
        switch (keyInput.key)
        {
        case Key::eT:
            ToggleRenderMode();
            break;
        default:
            break;
        }
    }
}

void Engine::ToggleRenderMode()
{
    uint32_t i = static_cast<const uint32_t>(state.renderMode);

    i = (i + 1) % kRenderModeCount;

    state.renderMode = static_cast<RenderMode>(i);
}
