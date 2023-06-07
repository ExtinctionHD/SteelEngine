#pragma once

#include "Engine/Render/Vulkan/Pipelines/PipelineBase.hpp"

struct ShaderModule;

class ComputePipeline : public PipelineBase
{
public:
    static std::unique_ptr<ComputePipeline> Create(const ShaderModule& shaderModule);

protected:
    ComputePipeline(vk::Pipeline pipeline_, vk::PipelineLayout layout_,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts_,
            const ShaderReflection& reflection_);

    vk::PipelineBindPoint GetBindPoint() const override { return vk::PipelineBindPoint::eCompute; }
};
