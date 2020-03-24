#pragma once

#include "Engine/Window.hpp"
#include "Engine/System.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Scene/Scene.hpp"

#include "Utils/TimeHelpers.hpp"

class Engine
{
public:
    Engine();
    ~Engine();

    void Run();

private:
    Timer timer;

    std::unique_ptr<Window> window;

    std::shared_ptr<Camera> camera;

    std::unique_ptr<Scene> scene;

    std::vector<std::unique_ptr<System>> systems;

    void ResizeCallback(const vk::Extent2D &extent) const;

    void KeyInputCallback(Key key, KeyAction action, ModifierFlags modifiers) const;

    void MouseInputCallback(MouseButton button, MouseButtonAction action, ModifierFlags modifiers) const;

    void MouseMoveCallback(const glm::vec2 &position) const;
};
