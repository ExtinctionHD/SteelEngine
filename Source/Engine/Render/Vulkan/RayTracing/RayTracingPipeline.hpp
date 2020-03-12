#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCache.hpp"
#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"

struct RayTracingShaderGroup
{
    vk::RayTracingShaderGroupTypeNV type;
    uint32_t generalShader;
    uint32_t closestHitShader;
    uint32_t anyHitShader;
};

struct RayTracingPipelineDescription
{
    std::vector<ShaderModule> shaderModules;
    std::vector<RayTracingShaderGroup> shaderGroups;
    std::vector<vk::DescriptorSetLayout> layouts;
    std::vector<vk::PushConstantRange> pushConstantRanges;
};

struct ShaderBindingTable
{
    BufferHandle buffer;
    vk::DeviceSize raygenOffset;
    vk::DeviceSize missOffset;
    vk::DeviceSize hitOffset;
    vk::DeviceSize stride;
};

class RayTracingPipeline
{
public:
    static std::unique_ptr<RayTracingPipeline> Create(std::shared_ptr<Device> device,
            BufferManager &bufferManager, const RayTracingPipelineDescription &description);

    ~RayTracingPipeline();

    vk::Pipeline Get() const { return pipeline; }

    vk::PipelineLayout GetLayout() const { return layout; }

    const ShaderBindingTable &GetShaderBindingTable() const { return shaderBindingTable; }

private:
    RayTracingPipeline(std::shared_ptr<Device> device_, vk::Pipeline pipeline_,
            vk::PipelineLayout layout_, const ShaderBindingTable &shaderBindingTable_);

    std::shared_ptr<Device> device;

    vk::Pipeline pipeline;

    vk::PipelineLayout layout;

    ShaderBindingTable shaderBindingTable;
};
