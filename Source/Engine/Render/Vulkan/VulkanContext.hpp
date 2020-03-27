#pragma once

#include "Engine/Render/Vulkan/Instance.hpp"
#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Surface.hpp"
#include "Engine/Render/Vulkan/Swapchain.hpp"
#include "Engine/Render/Vulkan/DescriptorPool.hpp"
#include "Engine/Render/Vulkan/Resources/MemoryManager.hpp"
#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"
#include "Engine/Render/Vulkan/Resources/ImageManager.hpp"
#include "Engine/Render/Vulkan/Resources/TextureCache.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCache.hpp"
#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureManager.hpp"

class Window;

class VulkanContext
{
public:
    static void Create(const Window &window);

    static std::unique_ptr<Instance> instance;
    static std::unique_ptr<Device> device;
    static std::unique_ptr<Surface> surface;
    static std::unique_ptr<Swapchain> swapchain;
    static std::unique_ptr<DescriptorPool> descriptorPool;
    static std::unique_ptr<MemoryManager> memoryManager;
    static std::unique_ptr<BufferManager> bufferManager;
    static std::unique_ptr<ImageManager> imageManager;
    static std::unique_ptr<TextureCache> textureCache;
    static std::unique_ptr<ShaderCache> shaderCache;
    static std::unique_ptr<AccelerationStructureManager> accelerationStructureManager;
};
