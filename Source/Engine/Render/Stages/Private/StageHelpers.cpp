#include "Engine/Render/Stages/StageHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"

CameraData StageHelpers::CreateCameraData(size_t bufferCount,
        vk::DeviceSize bufferSize, vk::ShaderStageFlags shaderStages)
{
    std::vector<vk::Buffer> buffers(bufferCount);

    const BufferDescription bufferDescription{
        bufferSize,
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    const DescriptorDescription descriptorDescription{
        1, vk::DescriptorType::eUniformBuffer,
        shaderStages,
        vk::DescriptorBindingFlags()
    };

    std::vector<DescriptorSetData> multiDescriptorSetData(bufferCount);

    for (size_t i = 0; i < bufferCount; ++i)
    {
        buffers[i] = VulkanContext::bufferManager->CreateBuffer(
                bufferDescription, BufferCreateFlagBits::eStagingBuffer);

        const DescriptorData descriptorData = DescriptorHelpers::GetData(buffers[i]);

        multiDescriptorSetData[i] = DescriptorSetData{ descriptorData };
    }

    const MultiDescriptorSet descriptorSet = DescriptorHelpers::CreateMultiDescriptorSet(
            { descriptorDescription }, multiDescriptorSetData);

    return CameraData{ buffers, descriptorSet };
}

vk::Rect2D StageHelpers::GetSwapchainRenderArea()
{
    return vk::Rect2D(vk::Offset2D(), VulkanContext::swapchain->GetExtent());
}

vk::Viewport StageHelpers::GetSwapchainViewport()
{
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    return vk::Viewport(0.0f, 0.0f,
            static_cast<float>(extent.width),
            static_cast<float>(extent.height),
            0.0f, 1.0f);
}
