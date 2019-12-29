#pragma once

#include <memory>

#include "Engine/Render/Vulkan/VulkanContext.hpp"

class Window;

class RenderSystem
{
public:
    RenderSystem(const Window &window);

    void Process() const;

private:
    std::unique_ptr<VulkanContext> vulkanContext;
};
