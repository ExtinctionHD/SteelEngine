#pragma once

#include "Engine/EngineHelpers.hpp"
#include "Engine/Scene/Scene.hpp"

#include "Utils/TimeHelpers.hpp"

class FrameLoop;
class Scene;
class Window;
class System;
class SceneRenderer;
class ImGuiRenderer;

class Engine
{
public:
    static const std::string kConfigPath;

    static void Create();
    static void Run();
    static void Destroy();

    static void TriggerEvent(EventType type);

    template <class T>
    static void TriggerEvent(EventType type, const T& argument);

    static void AddEventHandler(EventType type, const std::function<void()>& handler);

    template <class T>
    static void AddEventHandler(EventType type, const std::function<void(const T&)>& handler);

private:
    static Timer timer;

    static bool drawingSuspended;

    static std::unique_ptr<Window> window;

    static std::unique_ptr<SceneRenderer> sceneRenderer;
    static std::unique_ptr<ImGuiRenderer> imGuiRenderer;

    static std::vector<std::unique_ptr<System>> systems;
    static std::map<EventType, std::vector<EventHandler>> eventMap; // TODO create EventDispatcher

    static std::unique_ptr<Scene> scene;

    template <class T, class ...Args>
    static void AddSystem(Args&&...args);

    static void HandleResizeEvent(const vk::Extent2D& extent);

    static void HandleKeyInputEvent(const KeyInput& keyInput);

    static void HandleMouseInputEvent(const MouseInput& mouseInput);

    static void OpenScene();
};

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
void Engine::AddEventHandler(EventType type, const std::function<void(const T&)>& handler)
{
    std::vector<EventHandler>& eventHandlers = eventMap[type];
    eventHandlers.emplace_back([handler](std::any argument)
        {
            handler(std::any_cast<T>(argument));
        });
}
