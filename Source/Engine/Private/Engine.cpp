#include "Engine/Engine.hpp"

#include "Engine/Config.hpp"
#include "Engine/ConsoleVariable.hpp"
#include "Engine/UI/ImGuiRenderer.hpp"
#include "Engine/Filesystem/Filesystem.hpp"
#include "Engine/Scene/Systems/AnimationSystem.hpp"
#include "Engine/Scene/Systems/TestSystem.hpp"
#include "Engine/Scene/Systems/CameraSystem.hpp"
#include "Engine/Render/FrameLoop.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"

namespace Details
{
    static const Filepath kConfigPath("~/EngineConfig.ini");

    static int32_t windowWidth = 1280;
    static CVarInt windowWidthCVar("window.Width", windowWidth);
    
    static int32_t windowHeight = 720;
    static CVarInt windowHeightCVar("window.Height", windowHeight);

    static int32_t windowMode = static_cast<int32_t>(Window::Mode::eBorderless);
    static CVarInt windowModeCVar("window.Mode", windowMode);

    static int32_t sceneUseDefault = 1;
    static CVarInt sceneUseDefaultCVar("scene.UseDefault", sceneUseDefault);

    static std::string sceneDefaultPath = "~/Assets/Scenes/CornellBox/CornellBox.gltf";
    static CVarString sceneDefaultPathCVar("scene.DefaultPath", sceneDefaultPath);
    
    static Filepath GetScenePath()
    {
        if (sceneUseDefault)
        {
            return Filepath(sceneDefaultPath);
        }
        else
        {
            const DialogDescription dialogDescription{
                "Select Scene File", Filepath("~/"),
                { "glTF Files", "*.gltf" }
            };

            const std::optional<Filepath> scenePath = Filesystem::ShowOpenDialog(dialogDescription);

            return scenePath.value_or(Filepath(sceneDefaultPath));
        }
    }
}

Timer Engine::timer;
bool Engine::drawingSuspended = false;
std::unique_ptr<Window> Engine::window;
std::unique_ptr<Scene> Engine::scene;
std::unique_ptr<SceneRenderer> Engine::sceneRenderer;
std::unique_ptr<ImGuiRenderer> Engine::imGuiRenderer;
std::vector<std::unique_ptr<System>> Engine::systems;
std::map<EventType, std::vector<EventHandler>> Engine::eventMap;

void Engine::Create()
{
    EASY_FUNCTION()

    CVarHelpers::LoadConfig(Details::kConfigPath);

    const vk::Extent2D windowExtent(Details::windowWidth, Details::windowHeight);
    const Window::Mode windowMode = static_cast<Window::Mode>(Details::windowMode);
        
    window = std::make_unique<Window>(windowExtent, windowMode);

    VulkanContext::Create(*window);
    ResourceContext::Create();
    RenderContext::Create();

    AddEventHandler<vk::Extent2D>(EventType::eResize, &Engine::HandleResizeEvent);
    AddEventHandler<KeyInput>(EventType::eKeyInput, &Engine::HandleKeyInputEvent);
    AddEventHandler<MouseInput>(EventType::eMouseInput, &Engine::HandleMouseInputEvent);

    sceneRenderer = std::make_unique<SceneRenderer>();
    imGuiRenderer = std::make_unique<ImGuiRenderer>(*window);

    AddSystem<TestSystem>();
    AddSystem<AnimationSystem>();
    AddSystem<CameraSystem>();

    OpenScene();
}

void Engine::Run()
{
    while (!window->ShouldClose())
    {
        EASY_BLOCK("Engine::Frame")

        window->PollEvents();

        const float deltaSeconds = timer.Tick();

        if (scene)
        {
            for (const auto& system : systems)
            {
                system->Process(*scene, deltaSeconds);
            }
        }
        
        if (drawingSuspended)
        {
            continue;
        }

        imGuiRenderer->Build(scene.get(), deltaSeconds);

        RenderContext::frameLoop->Draw([](vk::CommandBuffer commandBuffer, uint32_t imageIndex)
            {
                sceneRenderer->Render(commandBuffer, imageIndex);
                imGuiRenderer->Render(commandBuffer, imageIndex);
            });
    }
}

void Engine::Destroy()
{
    VulkanContext::device->WaitIdle();

    systems.clear();

    imGuiRenderer.reset();
    sceneRenderer.reset();

    scene.reset();
    window.reset();

    RenderContext::Destroy();
    ResourceContext::Destroy();
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

    drawingSuspended = extent.width == 0 || extent.height == 0;

    if (!drawingSuspended)
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
        case Key::eO:
            OpenScene();
            break;
        default:
            break;
        }
    }
}

void Engine::HandleMouseInputEvent(const MouseInput& mouseInput)
{
    if (mouseInput.button == Config::DefaultCamera::kControlMouseButton)
    {
        switch (mouseInput.action)
        {
        case MouseButtonAction::ePress:
            window->SetCursorMode(Window::CursorMode::eDisabled);
            break;
        case MouseButtonAction::eRelease:
            window->SetCursorMode(Window::CursorMode::eEnabled);
            break;
        default:
            break;
        }
    }
}

void Engine::OpenScene()
{
    EASY_FUNCTION()

    VulkanContext::device->WaitIdle();

    sceneRenderer->RemoveScene();

    scene = std::make_unique<Scene>(Details::GetScenePath());

    for (const auto& system : systems)
    {
        system->Process(*scene, 0.0f);
    }

    sceneRenderer->RegisterScene(scene.get());
}
