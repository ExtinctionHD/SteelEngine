#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Window.hpp"
#include "Utils/Helpers.hpp"

namespace SVulkanContext
{
    std::vector<const char*> GetRequiredExtensions()
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
    const eValidation validation = eValidation::kDisabled;
#else
    const eValidation validation = eValidation::kEnabled;
#endif

    instance = Instance::Create(SVulkanContext::GetRequiredExtensions(), validation);
    surface = Surface::Create(instance, window.Get());
    device = Device::Create(instance, surface->Get(), SVulkanContext::kRequiredDeviceExtensions);

    resourceUpdateSystem = std::make_shared<ResourceUpdateSystem>(device, Numbers::kGigabyte);
    imageManager = std::make_shared<ImageManager>(device, resourceUpdateSystem);
    bufferManager = std::make_unique<BufferManager>(device, resourceUpdateSystem);
    textureCache = std::make_unique<TextureCache>(device, imageManager);

    swapchain = Swapchain::Create(device, surface->Get(), window);
    descriptorPool = DescriptorPool::Create(device, {
        vk::DescriptorType::eUniformBuffer,
        vk::DescriptorType::eCombinedImageSampler
    });

    shaderCache = std::make_unique<ShaderCache>(device, Filepath("~/Shaders/"));
}
