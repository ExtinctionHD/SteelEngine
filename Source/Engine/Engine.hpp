#pragma once

#include "Engine/Window.hpp"
#include "Engine/System.hpp"
#include "Engine/Camera.hpp"
#include "Engine/EngineHelpers.hpp"
#include "Engine/Scene/Scene.hpp"

#include "Utils/TimeHelpers.hpp"

class Engine
{
public:
    Engine();
    ~Engine();

    void Run();

private:
    EngineState state;
    Timer timer;

    std::unique_ptr<Window> window;
    std::unique_ptr<Scene> scene;
    std::unique_ptr<Camera> camera;
    std::vector<std::unique_ptr<System>> systems;

    void ResizeCallback(const vk::Extent2D &extent) const;

    void KeyInputCallback(Key key, KeyAction action, ModifierFlags modifiers) const;

    void MouseInputCallback(MouseButton button, MouseButtonAction action, ModifierFlags modifiers) const;

    void MouseMoveCallback(const glm::vec2 &position) const;

    template <class T, class ...Args>
    void AddSystem(Args &&...args);

    template <class T>
    T *GetSystem();
};

template <class T, class ...Args>
void Engine::AddSystem(Args &&...args)
{
    systems.emplace_back(new T(std::forward<Args>(args)...));
}

template <class T>
T *Engine::GetSystem()
{
    for (const auto &system : systems)
    {
        T *result = dynamic_cast<T *>(system.get());
        if (result != nullptr)
        {
            return result;
        }
    }

    return nullptr;
}
