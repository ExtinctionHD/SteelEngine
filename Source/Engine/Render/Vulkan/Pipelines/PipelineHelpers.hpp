#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

namespace PipelineHelpers
{
    glm::uvec3 CalculateWorkGroupCount(const vk::Extent3D& extent, const glm::uvec3& workGroupSize);

    glm::uvec3 CalculateWorkGroupCount(const vk::Extent2D& extent, const glm::uvec2& workGroupSize);

    uint32_t CalculateVertexSize(const VertexFormat& vertexFormat);
}
