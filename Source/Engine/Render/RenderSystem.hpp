#pragma once

#include <memory>

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/ForwardRenderPass.hpp"
#include "Engine/Render/RenderLoop.hpp"

class Window;

class RenderSystem
{
public:
    RenderSystem(const Window &window);

    void Process() const;

    void Draw() const;

private:
    std::unique_ptr<VulkanContext> vulkanContext;
    std::unique_ptr<ForwardRenderPass> renderPass;
    std::unique_ptr<RenderLoop> renderLoop;
};
