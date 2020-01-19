#pragma once

#include <optional>

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderModule.hpp"

struct VertexDescription
{
    std::vector<vk::Format> attributes;
    vk::VertexInputRate inputRate;
};

struct GraphicsPipelineProperties
{
    vk::Extent2D extent;
    vk::PrimitiveTopology topology;
    vk::PolygonMode polygonMode;
    vk::SampleCountFlagBits sampleCount;
    std::optional<vk::CompareOp> depthTest;
    std::vector<ShaderModule> shaderModules;
    std::vector<VertexDescription> vertexDescriptions;
    std::vector<vk::PipelineColorBlendAttachmentState> blendState;
    std::vector<vk::DescriptorSetLayout> layouts;
    std::vector<vk::PushConstantRange> pushConstantRanges;
};

class GraphicsPipeline
{
public:
    static std::unique_ptr<GraphicsPipeline> Create(std::shared_ptr<Device> device,
            vk::RenderPass renderPass, const GraphicsPipelineProperties &properties);

    GraphicsPipeline(std::shared_ptr<Device> aDevice, vk::Pipeline aPipeline, vk::PipelineLayout aLayout);
    ~GraphicsPipeline();

    vk::Pipeline Get() const { return pipeline; }

    vk::PipelineLayout GetLayout() const { return layout; }

private:
    std::shared_ptr<Device> device;

    vk::Pipeline pipeline;

    vk::PipelineLayout layout;
};
