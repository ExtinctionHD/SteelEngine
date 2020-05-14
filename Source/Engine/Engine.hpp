#pragma once

#include "Engine/Window.hpp"
#include "Engine/System.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Scene/Scene.hpp"

#include "Utils/Timer.hpp"

class Engine
{
public:
    static void Create();

    static void Run();

private:
    static Timer timer;

    static std::unique_ptr<Window> window;
    static std::unique_ptr<Scene> scene;
    static std::unique_ptr<Camera> camera;

    static std::vector<std::unique_ptr<System>> systems;

    static void ResizeCallback(const vk::Extent2D &extent);

    static void KeyInputCallback(Key key, KeyAction action, ModifierFlags modifiers);

    static void MouseInputCallback(MouseButton button, MouseButtonAction action, ModifierFlags modifiers);

    static void MouseMoveCallback(const glm::vec2 &position);

    template <class T, class ...Args>
    static void AddSystem(Args &&...args);

    template <class T>
    static T *GetSystem();
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
