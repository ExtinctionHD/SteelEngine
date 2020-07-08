#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"
#include "Engine/Window.hpp"
#include "Engine/Config.hpp"

namespace Details
{
    void InitializeDefaultDispatcher()
    {
        const vk::DynamicLoader dynamicLoader;
        const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr
                = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    }

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
std::unique_ptr<ShaderManager> VulkanContext::shaderManager;
std::unique_ptr<MemoryManager> VulkanContext::memoryManager;
std::unique_ptr<BufferManager> VulkanContext::bufferManager;
std::unique_ptr<ImageManager> VulkanContext::imageManager;
std::unique_ptr<TextureManager> VulkanContext::textureManager;
std::unique_ptr<AccelerationStructureManager> VulkanContext::accelerationStructureManager;

void VulkanContext::Create(const Window &window)
{
    Details::InitializeDefaultDispatcher();

    const std::vector<const char *> requiredExtensions
            = Details::UpdateRequiredExtensions(VulkanConfig::kRequiredExtensions);

    instance = Instance::Create(requiredExtensions);
    surface = Surface::Create(window.Get());
    device = Device::Create(VulkanConfig::kRequiredDeviceFeatures, VulkanConfig::kRequiredDeviceExtensions);
    swapchain = Swapchain::Create(Swapchain::Description{ window.GetExtent(), Config::kVSyncEnabled });
    descriptorPool = DescriptorPool::Create(VulkanConfig::kMaxDescriptorSetCount, VulkanConfig::kDescriptorPoolSizes);

    shaderManager = std::make_unique<ShaderManager>(Config::kShadersDirectory);
    memoryManager = std::make_unique<MemoryManager>();
    bufferManager = std::make_unique<BufferManager>();
    imageManager = std::make_unique<ImageManager>();
    textureManager = std::make_unique<TextureManager>();
    accelerationStructureManager = std::make_unique<AccelerationStructureManager>();
}

void VulkanContext::Destroy()
{
    accelerationStructureManager.reset();
    textureManager.reset();
    imageManager.reset();
    bufferManager.reset();
    memoryManager.reset();
    shaderManager.reset();
    descriptorPool.reset();
    swapchain.reset();
    device.reset();
    surface.reset();
    instance.reset();
}
