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
    swapchain = Swapchain::Create(device, surface->Get(), window);
    descriptorPool = DescriptorPool::Create(device, {
        vk::DescriptorType::eUniformBuffer,
        vk::DescriptorType::eCombinedImageSampler
    });

    transferSystem = std::make_shared<TransferSystem>(device, 1 * 1024 * 1024);
    bufferManager = std::make_unique<BufferManager>(device, transferSystem);
    imageManager = std::make_unique<ImageManager>(device, transferSystem);

    shaderCache = std::make_unique<ShaderCache>(device, Filepath("~/Shaders/"));
}
