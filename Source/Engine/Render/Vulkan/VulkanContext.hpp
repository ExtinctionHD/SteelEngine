#pragma once

#include <memory>

#include "Engine/Render/Vulkan/Instance.hpp"
#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Surface.hpp"
#include "Engine/Render/Vulkan/Swapchain.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"

#include "Engine/Render/Vulkan/Resources/ImagePool.hpp"
#include "Engine/Render/Vulkan/Resources/BufferPool.hpp"

class Window;

class VulkanContext
{
public:
    VulkanContext(const Window &window);

private:
    std::shared_ptr<Instance> instance;
    std::shared_ptr<Device> device;

    std::unique_ptr<Surface> surface;
    std::unique_ptr<Swapchain> swapchain;
    std::unique_ptr<RenderPass> renderPass;

    std::unique_ptr<ImagePool> imagePool;
    std::unique_ptr<BufferPool> bufferPool;
};
