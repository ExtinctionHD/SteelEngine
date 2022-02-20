#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

struct CameraData
{
    std::vector<vk::Buffer> buffers;
    MultiDescriptorSet descriptorSet;
};

namespace RenderHelpers
{
    CameraData CreateCameraData(uint32_t bufferCount,
            vk::DeviceSize bufferSize, vk::ShaderStageFlags shaderStages);

    vk::Rect2D GetSwapchainRenderArea();

    vk::Viewport GetSwapchainViewport();
}
