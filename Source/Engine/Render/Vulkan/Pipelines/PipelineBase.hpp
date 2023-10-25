#pragma once

#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"
#include "Utils/Assert.hpp"

class PipelineBase
{
public:
    virtual ~PipelineBase();

    vk::Pipeline Get() const
    {
        return pipeline;
    }

    vk::PipelineLayout GetLayout() const
    {
        return layout;
    }

    const std::vector<vk::DescriptorSetLayout>& GetDescriptorSetLayouts() const
    {
        return descriptorSetLayouts;
    }

    void Bind(vk::CommandBuffer commandBuffer) const;

    void BindDescriptorSet(
        vk::CommandBuffer commandBuffer, uint32_t firstSet, vk::DescriptorSet descriptorSet) const;

    void BindDescriptorSets(
        vk::CommandBuffer commandBuffer, const std::vector<vk::DescriptorSet>& descriptorSets) const;

    template <class T>
    void PushConstant(vk::CommandBuffer commandBuffer, const std::string& name, const T& value) const;

    std::unique_ptr<DescriptorProvider> CreateDescriptorProvider() const;

protected:
    PipelineBase(vk::Pipeline pipeline_, vk::PipelineLayout layout_, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts_, const ShaderReflection& reflection_);

    virtual vk::PipelineBindPoint GetBindPoint() const = 0;

private:
    vk::Pipeline pipeline;
    vk::PipelineLayout layout;

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;

    ShaderReflection reflection;
};

template <class T>
void PipelineBase::PushConstant(
    vk::CommandBuffer commandBuffer, const std::string& name, const T& value) const
{
    Assert(reflection.pushConstants.contains(name));

    const vk::PushConstantRange& pushConstantRange = reflection.pushConstants.at(name);

    commandBuffer.pushConstants<T>(layout, pushConstantRange.stageFlags, pushConstantRange.offset, { value });
}
