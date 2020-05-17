#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"
#include "Engine/Window.hpp"
#include "Engine/Config.hpp"

namespace SVulkanContext
{
    std::vector<const char *> UpdateRequiredExtensions(const std::vector<const char *> &requiredExtension)
    {
        uint32_t count = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + count);
        extensions.reserve(extensions.size() + requiredExtension.size());

        std::copy(requiredExtension.begin(), requiredExtension.end(), extensions.end());

        return extensions;
    }
}

std::unique_ptr<Instance> VulkanContext::instance;
std::unique_ptr<Device> VulkanContext::device;
std::unique_ptr<Surface> VulkanContext::surface;
std::unique_ptr<Swapchain> VulkanContext::swapchain;
std::unique_ptr<DescriptorPool> VulkanContext::descriptorPool;
std::unique_ptr<MemoryManager> VulkanContext::memoryManager;
std::unique_ptr<BufferManager> VulkanContext::bufferManager;
std::unique_ptr<ImageManager> VulkanContext::imageManager;
std::unique_ptr<TextureCache> VulkanContext::textureCache;
std::unique_ptr<ShaderCache> VulkanContext::shaderCache;
std::unique_ptr<AccelerationStructureManager> VulkanContext::accelerationStructureManager;

void VulkanContext::Create(const Window &window)
{
    const std::vector<const char*> requiredExtensions
            = SVulkanContext::UpdateRequiredExtensions(VulkanConfig::kRequiredExtensions);

    instance = Instance::Create(requiredExtensions);
    surface = Surface::Create(window.Get());
    device = Device::Create(VulkanConfig::kRequiredDeviceFeatures, VulkanConfig::kRequiredDeviceExtensions);
    swapchain = Swapchain::Create(Swapchain::Description{ window.GetExtent(), Config::kVSyncEnabled });
    descriptorPool = DescriptorPool::Create(VulkanConfig::kMaxDescriptorSetCount, VulkanConfig::kDescriptorPoolSizes);
    memoryManager = std::make_unique<MemoryManager>();
    bufferManager = std::make_unique<BufferManager>();
    imageManager = std::make_unique<ImageManager>();
    textureCache = std::make_unique<TextureCache>();
    shaderCache = std::make_unique<ShaderCache>(Config::kShadersDirectory);
    accelerationStructureManager = std::make_unique<AccelerationStructureManager>();

    ShaderCompiler::Initialize();
}

void VulkanContext::Destroy()
{
    accelerationStructureManager.reset(nullptr);
    shaderCache.reset(nullptr);
    textureCache.reset(nullptr);
    imageManager.reset(nullptr);
    bufferManager.reset(nullptr);
    memoryManager.reset(nullptr);
    descriptorPool.reset(nullptr);
    swapchain.reset(nullptr);
    device.reset(nullptr);
    surface.reset(nullptr);
    instance.reset(nullptr);
}
