#pragma once

#include "Engine/Render/Vulkan/Pipelines/PipelineBase.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

struct VertexInput
{
    VertexFormat format;
    uint32_t stride;
    vk::VertexInputRate inputRate;
};

enum class BlendMode
{
    eDisabled,
    eAlphaBlend,
};

class GraphicsPipeline : public PipelineBase
{
public:
    struct Description
    {
        vk::PrimitiveTopology topology;
        vk::PolygonMode polygonMode;
        vk::CullModeFlagBits cullMode;
        vk::FrontFace frontFace;
        vk::SampleCountFlagBits sampleCount;
        std::optional<vk::CompareOp> depthTest;
        std::vector<ShaderModule> shaderModules;
        std::vector<VertexInput> vertexInputs;
        std::vector<BlendMode> blendModes;
    };

    static std::unique_ptr<GraphicsPipeline> Create(
        vk::RenderPass renderPass, const Description& description);

protected:
    GraphicsPipeline(vk::Pipeline pipeline_, vk::PipelineLayout layout_, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts_, const ShaderReflection& reflection_);

    vk::PipelineBindPoint GetBindPoint() const override
    {
        return vk::PipelineBindPoint::eGraphics;
    }
};
