#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"

struct ShaderBindingTable
{
    vk::Buffer buffer;
    vk::DeviceSize raygenOffset;
    vk::DeviceSize missOffset;
    vk::DeviceSize hitOffset;
    vk::DeviceSize stride;
};

enum class ShaderGroupType
{
    eRaygen,
    eMiss,
    eHit
};

struct ShaderGroup
{
    uint32_t generalShader;
    uint32_t closestHitShader;
    uint32_t anyHitShader;
    uint32_t intersectionShader;
};

class RayTracingPipeline
{
public:
    struct Description
    {
        std::vector<ShaderModule> shaderModules;
        std::map<ShaderGroupType, std::vector<ShaderGroup>> shaderGroups;

        std::vector<vk::DescriptorSetLayout> layouts;
        std::vector<vk::PushConstantRange> pushConstantRanges;
    };

    static std::unique_ptr<RayTracingPipeline> Create(const Description& description);

    ~RayTracingPipeline();

    vk::Pipeline Get() const { return pipeline; }

    vk::PipelineLayout GetLayout() const { return layout; }

    const ShaderBindingTable& GetShaderBindingTable() const { return shaderBindingTable; }

private:
    RayTracingPipeline(vk::Pipeline pipeline_, vk::PipelineLayout layout_,
            const ShaderBindingTable& shaderBindingTable_);

    vk::Pipeline pipeline;

    vk::PipelineLayout layout;

    ShaderBindingTable shaderBindingTable;
};
