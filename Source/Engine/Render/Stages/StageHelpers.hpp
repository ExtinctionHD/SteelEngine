#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

struct CameraData
{
    std::vector<vk::Buffer> buffers;
    MultiDescriptorSet descriptorSet;
};

namespace StageHelpers
{
    CameraData CreateCameraData(size_t bufferCount,
            vk::DeviceSize bufferSize, vk::ShaderStageFlags shaderStages);

    vk::Rect2D GetSwapchainRenderArea();

    vk::Viewport GetSwapchainViewport();
}
