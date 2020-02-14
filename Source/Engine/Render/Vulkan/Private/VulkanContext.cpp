#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Window.hpp"
#include "Engine/Config.hpp"

namespace SVulkanContext
{
    std::vector<const char*> UpdateRequiredExtensions(const std::vector<const char *> &requiredExtension)
    {
        uint32_t count = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + count);
        extensions.reserve(extensions.size() + requiredExtension.size());

        std::copy(requiredExtension.begin(), requiredExtension.end(), extensions.end());

        return extensions;
    }
}

VulkanContext::VulkanContext(const Window &window)
{
    const std::vector<const char*> requiredExtensions
            = SVulkanContext::UpdateRequiredExtensions(VulkanConfig::kRequiredExtensions);

    instance = Instance::Create(requiredExtensions, VulkanConfig::kVulkanValidation);
    surface = Surface::Create(instance, window.Get());
    device = Device::Create(instance, surface->Get(),
            VulkanConfig::kRequiredDeviceExtensions, VulkanConfig::kRequiredDeviceFeatures);

    resourceUpdateSystem = std::make_shared<ResourceUpdateSystem>(device, VulkanConfig::kStagingBufferCapacity);
    imageManager = std::make_shared<ImageManager>(device, resourceUpdateSystem);
    bufferManager = std::make_unique<BufferManager>(device, resourceUpdateSystem);
    textureCache = std::make_unique<TextureCache>(device, imageManager);

    swapchain = Swapchain::Create(device, { surface->Get(), window.GetExtent() });

    descriptorPool = DescriptorPool::Create(device, VulkanConfig::kDescriptorPoolSizes,
            VulkanConfig::kMaxDescriptorSetCount);

    shaderCache = std::make_unique<ShaderCache>(device, Config::kShadersDirectory);
}
