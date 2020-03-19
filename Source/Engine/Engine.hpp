#pragma once

#include "Engine/Window.hpp"
#include "Engine/System.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

class Engine
{
public:
    Engine();
    ~Engine();

    void Run() const;

private:
    std::unique_ptr<Window> window;

    std::shared_ptr<VulkanContext> vulkanContext;

    std::list<std::unique_ptr<System>> systems;

    void ResizeCallback(const vk::Extent2D &extent) const;

    void KeyInputCallback(Key key, KeyAction action, ModifierFlags modifiers) const;

    void MouseInputCallback(MouseButton button, MouseButtonAction action, ModifierFlags modifiers) const;

    void MouseMoveCallback(const glm::vec2 &position) const;
};
