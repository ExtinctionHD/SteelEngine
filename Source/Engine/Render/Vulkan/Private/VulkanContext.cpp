#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Window.hpp"

namespace SVulkanContext
{
    std::vector<const char *> GetRequiredExtensions()
    {
        uint32_t count = 0;
        const char **extensions = glfwGetRequiredInstanceExtensions(&count);

        return std::vector<const char*>(extensions, extensions + count);
    }

    const std::vector<const char*> kRequiredDeviceExtensions
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        //VK_NV_RAY_TRACING_EXTENSION_NAME
    };
}

VulkanContext::VulkanContext(const Window &window)
{
#ifdef NDEBUG
    const Instance::eValidation validation = Instance::eValidation::kDisabled;
#else
    const Instance::eValidation validation = Instance::eValidation::kEnabled;
#endif

    instance = Instance::Create(SVulkanContext::GetRequiredExtensions(), validation);
    surface = Surface::Create(instance, window.Get());
    device = Device::Create(instance, surface->Get(), SVulkanContext::kRequiredDeviceExtensions);

    resourceUpdateSystem = std::make_shared<ResourceUpdateSystem>(device, 1 * 1024 * 1024);
    bufferManager = std::make_unique<BufferManager>(device, resourceUpdateSystem);
    imageManager = std::make_unique<ImageManager>(device, resourceUpdateSystem);

    swapchain = Swapchain::Create(device, surface->Get(), window);
    descriptorPool = DescriptorPool::Create(device, {
        vk::DescriptorType::eUniformBuffer,
        vk::DescriptorType::eCombinedImageSampler
    });

    shaderCache = std::make_unique<ShaderCache>(device, Filepath("~/Shaders/"));
}
