#pragma once

#include "Utils/Assert.hpp"

class PipelineBase
{
public:
    virtual ~PipelineBase();

    vk::Pipeline Get() const { return pipeline; }

    vk::PipelineLayout GetLayout() const { return layout; }

    void Bind(vk::CommandBuffer commandBuffer) const;

    void BindDescriptorSets(vk::CommandBuffer commandBuffer,
            uint32_t firstSet, const std::vector<vk::DescriptorSet>& descriptorSets) const;

    template <class T>
    void PushConstant(vk::CommandBuffer commandBuffer, const std::string& name, const T& value) const;

    const std::vector<vk::DescriptorSetLayout>& GetDescriptorSetLayouts() const { return descriptorSetLayouts; }

protected:
    PipelineBase(vk::Pipeline pipeline_, vk::PipelineLayout layout_,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts_,
            const std::map<std::string, vk::PushConstantRange>& pushConstants_);

    virtual vk::PipelineBindPoint GetBindPoint() const = 0;

private:
    vk::Pipeline pipeline;
    vk::PipelineLayout layout;

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    std::map<std::string, vk::PushConstantRange> pushConstants;
};

template <class T>
void PipelineBase::PushConstant(vk::CommandBuffer commandBuffer, const std::string& name, const T& value) const
{
    Assert(pushConstants.contains(name));

    const vk::PushConstantRange& pushConstantRange = pushConstants.at(name);

    commandBuffer.pushConstants<T>(layout, pushConstantRange.stageFlags, pushConstantRange.offset, { value });
}
