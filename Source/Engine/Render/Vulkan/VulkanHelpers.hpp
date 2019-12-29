#pragma once

#include "Engine/Render/Vulkan/Vulkan.hpp"

namespace VulkanHelpers
{
    const vk::ComponentMapping kComponentMappingRgba(
        vk::ComponentSwizzle::eR,
        vk::ComponentSwizzle::eG,
        vk::ComponentSwizzle::eB,
        vk::ComponentSwizzle::eA);

    const vk::ImageSubresourceRange kSubresourceRangeDefault(
        vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
}