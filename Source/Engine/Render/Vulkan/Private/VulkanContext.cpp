#include "Engine/Render/Vulkan/VulkanContext.hpp"

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
    const Instance::eValidation validation = Instance::eValidation::kDisabled;
#else
    const Instance::eValidation validation = Instance::eValidation::kEnabled;
#endif

    instance = Instance::Create(SVulkanContext::GetRequiredExtensions(), validation);
    surface = Surface::Create(instance, window.Get());
    device = Device::Create(instance, surface->Get(),
            SVulkanContext::kRequiredDeviceExtensions);
    swapchain = Swapchain::Create(device, surface->Get(), window);

    // Render pass test
    const RenderPass::Attachment attachment{
        RenderPass::Attachment::eUsage::kColor,
        swapchain->GetFormat(),
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR
    };
    renderPass = RenderPass::Create(device, { attachment },
            vk::SampleCountFlagBits::e1, vk::PipelineBindPoint::eGraphics);

    transferSystem = TransferSystem::Create(device);
    imageManager = ImageManager::Create(device);
    bufferManager = BufferManager::Create(device, transferSystem);

    // Image manager test
    const ImageProperties imageProperties{
        eImageType::k3D, vk::Format::eR16Sfloat, vk::Extent3D(1024, 1024, 1), 1, 1,
        vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment,
        vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal
    };
    const vk::ImageSubresourceRange subresourceRange{
        vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
    };
    ImageData testImage = imageManager->CreateImage(imageProperties);
    testImage = imageManager->CreateView(testImage, subresourceRange);
    Assert(testImage.GetType() == eImageDataType::kImageWithView);

    // Buffer manager test
    const BufferProperties bufferProperties{
        sizeof(float) * 3, vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };
    BufferData testBuffer = bufferManager->CreateBuffer(bufferProperties, std::vector<float>{ 1.0f, 2.0f, 3.0f });
    Assert(testBuffer.GetType() == eBufferDataType::kNeedUpdate);

    bufferManager->UpdateMarkedBuffers();
    transferSystem->PerformTransfer();

    // Descriptor pool test
    descriptorPool = DescriptorPool::Create(device, {
        vk::DescriptorType::eUniformBuffer, vk::DescriptorType::eCombinedImageSampler
    });
    const DescriptorSetProperties descriptorSetProperties{
        { vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex },
        { vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment }
    };
    vk::DescriptorSetLayout descriptorSetLayout = descriptorPool->CreateDescriptorSetLayout(descriptorSetProperties);
    vk::DescriptorSet descriptorSet = descriptorPool->AllocateDescriptorSet(descriptorSetLayout);
    const DescriptorSetData descriptorSetData{
        {
            vk::DescriptorType::eUniformBuffer,
            vk::DescriptorBufferInfo(testBuffer.GetBuffer(), 0, testBuffer.GetProperties().size)
        },
        {
            vk::DescriptorType::eCombinedImageSampler,
            vk::DescriptorImageInfo(vk::Sampler(), testImage.GetView(), vk::ImageLayout::eShaderReadOnlyOptimal)
        }
    };
    descriptorPool->UpdateDescriptorSet(descriptorSet, descriptorSetData);
    descriptorPool->PerformUpdate();
}
