#pragma once

#include <memory>

#include "Engine/Render/Vulkan/VulkanContext.hpp"

struct GLFWwindow;

class RenderSystem
{
public:
    RenderSystem(GLFWwindow *window);

    void Process() const;

private:
    std::unique_ptr<VulkanContext> vulkanContext;
};
