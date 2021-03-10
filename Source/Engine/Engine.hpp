#pragma once

#include "Engine/EngineHelpers.hpp"
#include "Engine/Systems/System.hpp"

#include "Utils/TimeHelpers.hpp"

class Camera;
class Environment;
class FrameLoop;
class SceneModel;
class ScenePT;
class Scene;
class Window;

class Engine
{
public:
    struct State
    {
        RenderMode renderMode = RenderMode::eHybrid;
        bool drawingSuspended = false;
    };

    static void Create();
    static void Run();
    static void Destroy();

    static const State& GetState() { return state; }

    template <class T>
    static T* GetSystem();

    static void TriggerEvent(EventType type);

    template <class T>
    static void TriggerEvent(EventType type, const T& argument);

    static void AddEventHandler(EventType type, std::function<void()> handler);

    template <class T>
    static void AddEventHandler(EventType type, std::function<void(const T&)> handler);

private:
    static Timer timer;
    static State state;

    static std::unique_ptr<Window> window;
    static std::unique_ptr<FrameLoop> frameLoop;

    static std::unique_ptr<SceneModel> sceneModel;
    static std::unique_ptr<Environment> environment;

    static std::unique_ptr<Scene> scene;
    static std::unique_ptr<ScenePT> scenePT;
    static std::unique_ptr<Camera> camera;

    static std::vector<std::unique_ptr<System>> systems;
    static std::map<EventType, std::vector<EventHandler>> eventMap;

    template <class T, class ...Args>
    static void AddSystem(Args&&...args);

    static void HandleResizeEvent(const vk::Extent2D& extent);

    static void HandleKeyInputEvent(const KeyInput& keyInput);

    static void ToggleRenderMode();
};

template <class T>
T* Engine::GetSystem()
{
    for (const auto& system : systems)
    {
        T* result = dynamic_cast<T*>(system.get());
        if (result != nullptr)
        {
            return result;
        }
    }

    return nullptr;
}

template <class T, class ...Args>
void Engine::AddSystem(Args&&...args)
{
    systems.emplace_back(new T(std::forward<Args>(args)...));
}

template <class T>
void Engine::TriggerEvent(EventType type, const T& argument)
{
    for (const auto& handler : eventMap[type])
    {
        handler(std::any(argument));
    }
}

template <class T>
void Engine::AddEventHandler(EventType type, std::function<void(const T&)> handler)
{
    std::vector<EventHandler>& eventHandlers = eventMap[type];
    eventHandlers.emplace_back([handler](std::any argument)
        {
            handler(std::any_cast<T>(argument));
        });
}
