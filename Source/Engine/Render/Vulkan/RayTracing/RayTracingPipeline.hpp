#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCache.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

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

class RayTracingPipeline
{
public:
    static std::unique_ptr<RayTracingPipeline> Create(std::shared_ptr<Device> device,
            const RayTracingPipelineDescription &description);

    ~RayTracingPipeline();

    vk::Pipeline Get() const { return pipeline; }

    vk::PipelineLayout GetLayout() const { return layout; }

    uint32_t GetShaderGroupCount() const { return shaderGroupCount; }

private:
    RayTracingPipeline(std::shared_ptr<Device> aDevice, vk::Pipeline aPipeline,
            vk::PipelineLayout aLayout, uint32_t aShaderGroupCount);

    std::shared_ptr<Device> device;

    vk::Pipeline pipeline;

    vk::PipelineLayout layout;

    uint32_t shaderGroupCount;
};
