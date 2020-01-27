#pragma once

#include <memory>

#include "Engine/Render/Vulkan/Instance.hpp"
#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Surface.hpp"
#include "Engine/Render/Vulkan/Swapchain.hpp"
#include "Engine/Render/Vulkan/DescriptorPool.hpp"
#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"
#include "Engine/Render/Vulkan/Resources/ImageManager.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCache.hpp"

class Window;

class VulkanContext
{
public:
    VulkanContext(const Window &window);

    std::shared_ptr<Instance> instance;
    std::shared_ptr<Device> device;

    std::unique_ptr<Surface> surface;
    std::unique_ptr<Swapchain> swapchain;
    std::unique_ptr<DescriptorPool> descriptorPool;

    std::shared_ptr<ResourceUpdateSystem> resourceUpdateSystem;
    std::unique_ptr<BufferManager> bufferManager;
    std::unique_ptr<ImageManager> imageManager;

    std::unique_ptr<ShaderCache> shaderCache;
};
