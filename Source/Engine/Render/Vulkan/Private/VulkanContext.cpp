#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Engine/Window.hpp"

#include "Utils/Assert.hpp"

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
        VK_NV_RAY_TRACING_EXTENSION_NAME
    };
}

VulkanContext::VulkanContext(const Window &window)
{
#ifdef NDEBUG
    const VulkanInstance::eValidation validation = VulkanInstance::eValidation::kDisabled;
#else
    const VulkanInstance::eValidation validation = VulkanInstance::eValidation::kEnabled;
#endif

    vulkanInstance = VulkanInstance::Create(SVulkanContext::GetRequiredExtensions(), validation);
    vulkanSurface = VulkanSurface::Create(vulkanInstance, window.Get());
    vulkanDevice = VulkanDevice::Create(vulkanInstance, vulkanSurface->Get(),
            SVulkanContext::kRequiredDeviceExtensions);
    vulkanSwapchain = VulkanSwapchain::Create(vulkanDevice, vulkanSurface->Get(), window);

    const VulkanRenderPass::Attachment attachment{
        VulkanRenderPass::Attachment::eUsage::kColor,
        vulkanSwapchain->GetFormat(),
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR
    };
    vulkanRenderPass = VulkanRenderPass::Create(vulkanDevice, { attachment },
            vk::SampleCountFlagBits::e1, vk::PipelineBindPoint::eGraphics);

    imageFactory = ImagePool::Create(vulkanDevice);
    
    const ImageProperties imageProperties{
        eImageType::k3D, vk::Format::eR16Sfloat, vk::Extent3D(1024, 1024, 1), 1, 1,
        vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment,
        vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal
    };
    const vk::ImageSubresourceRange subresourceRange{
        vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
    };
    ImageData testImage = imageFactory->CreateImage(imageProperties);
    testImage = imageFactory->CreateView(testImage, subresourceRange);
    Assert(testImage.GetType() == eImageDataType::kImageWithView);
}
