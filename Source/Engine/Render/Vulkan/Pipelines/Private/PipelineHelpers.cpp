#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"

#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

glm::uvec3 PipelineHelpers::CalculateWorkGroupCount(const vk::Extent3D& extent, const glm::uvec3& workGroupSize)
{
    const auto calculate = [](uint32_t dimension, uint32_t groupSize)
        {
            return dimension / groupSize + std::min(dimension % groupSize, 1u);
        };

    glm::uvec3 groupCount;

    groupCount.x = calculate(extent.width, workGroupSize.x);
    groupCount.y = calculate(extent.height, workGroupSize.y);
    groupCount.z = calculate(extent.depth, workGroupSize.z);

    return groupCount;
}

glm::uvec3 PipelineHelpers::CalculateWorkGroupCount(const vk::Extent2D& extent, const glm::uvec2& workGroupSize)
{
    return CalculateWorkGroupCount(VulkanHelpers::GetExtent3D(extent), glm::uvec3(workGroupSize.x, workGroupSize.y, 1));
}
