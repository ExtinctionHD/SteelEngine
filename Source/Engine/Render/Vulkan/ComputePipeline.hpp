#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderCache.hpp"

struct ComputePipelineDescription
{
    vk::Extent2D extent;
    ShaderModule shaderModule;
    std::vector<vk::DescriptorSetLayout> layouts;
    std::vector<vk::PushConstantRange> pushConstantRanges;
};

class ComputePipeline
{
public:
    static std::unique_ptr<ComputePipeline> Create(const ComputePipelineDescription &description);

    ~ComputePipeline();

    vk::Pipeline Get() const { return pipeline; }

    vk::PipelineLayout GetLayout() const { return layout; }

private:
    vk::Pipeline pipeline;

    vk::PipelineLayout layout;

    ComputePipeline(vk::Pipeline pipeline_, vk::PipelineLayout layout_);
};
