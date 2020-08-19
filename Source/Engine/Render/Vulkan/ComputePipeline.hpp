#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"

class ComputePipeline
{
public:
    struct Description
    {
        ShaderModule shaderModule;
        std::vector<vk::DescriptorSetLayout> layouts;
        std::vector<vk::PushConstantRange> pushConstantRanges;
    };

    static std::unique_ptr<ComputePipeline> Create(const Description& description);

    ~ComputePipeline();

    vk::Pipeline Get() const { return pipeline; }

    vk::PipelineLayout GetLayout() const { return layout; }

private:
    vk::Pipeline pipeline;

    vk::PipelineLayout layout;

    ComputePipeline(vk::Pipeline pipeline_, vk::PipelineLayout layout_);
};
