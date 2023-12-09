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

using ShaderGroupMap = std::map<ShaderGroupType, std::vector<ShaderGroup>>;

class RayTracingPipeline : public PipelineBase
{
public:
    struct Description
    {
        std::vector<ShaderModule> shaderModules;
        ShaderGroupMap shaderGroupMap;
    };

    ~RayTracingPipeline() override;

    static std::unique_ptr<RayTracingPipeline> Create(const Description& description);

    void TraceRays(vk::CommandBuffer commandBuffer, const vk::Extent3D& extent) const;

    void GenerateShaderBindingTable();

protected:
    RayTracingPipeline(vk::Pipeline pipeline_, vk::PipelineLayout layout_,
            const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts_,
            const ShaderReflection& reflection_, const ShaderGroupMap& shaderGroupMap);

    vk::PipelineBindPoint GetBindPoint() const override { return vk::PipelineBindPoint::eRayTracingKHR; }

private:
    ShaderGroupMap shaderGroupMap;

    ShaderBindingTable shaderBindingTable;
};
