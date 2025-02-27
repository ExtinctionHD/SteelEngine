#pragma once

// TODO rename to ComputeHelpers
namespace PipelineHelpers
{
    static constexpr glm::uvec3 kDefaultWorkGroupSize(8, 8, 1);

    glm::uvec3 CalculateWorkGroupCount(const vk::Extent3D& extent,
            const glm::uvec3& workGroupSize = kDefaultWorkGroupSize);

    glm::uvec3 CalculateWorkGroupCount(const vk::Extent2D& extent,
            const glm::uvec2& workGroupSize = kDefaultWorkGroupSize);
}
