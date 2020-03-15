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

    void WindowResizeCallback(const vk::Extent2D &extent) const;
};
