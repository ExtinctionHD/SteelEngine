#pragma once

#include "Engine/Render/Vulkan/Pipelines/PipelineBase.hpp"
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

class RayTracingPipeline : public PipelineBase
{
public:
    struct Description
    {
        std::vector<ShaderModule> shaderModules;
        std::map<ShaderGroupType, std::vector<ShaderGroup>> shaderGroups;
    };

    ~RayTracingPipeline() override;

    static std::unique_ptr<RayTracingPipeline> Create(const Description& description);

    const ShaderBindingTable& GetShaderBindingTable() const
    {
        return shaderBindingTable;
    }

protected:
    RayTracingPipeline(vk::Pipeline pipeline_, vk::PipelineLayout layout_, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts_, const ShaderReflection& reflection_, const ShaderBindingTable& shaderBindingTable_);

    vk::PipelineBindPoint GetBindPoint() const override
    {
        return vk::PipelineBindPoint::eRayTracingKHR;
    }

private:
    ShaderBindingTable shaderBindingTable;
};
