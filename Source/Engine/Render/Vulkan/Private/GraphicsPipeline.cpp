#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Assert.hpp"

namespace SGraphicsPipeline
{
    std::vector<vk::PipelineShaderStageCreateInfo> ObtainShaderStagesCreateInfo(
            const std::vector<ShaderModule> &shaderModules)
    {
        std::vector<vk::PipelineShaderStageCreateInfo> createInfo;
        createInfo.reserve(shaderModules.size());

        for (const auto &shaderModule : shaderModules)
        {
            createInfo.emplace_back(vk::PipelineShaderStageCreateFlags(),
                    shaderModule.stage, shaderModule.module, "main", nullptr);
        }

        return createInfo;
    }

    vk::PipelineVertexInputStateCreateInfo ObtainVertexInputStateCreateInfo(
            const std::vector<VertexDescription> &vertexDescriptions)
    {
        static std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
        static std::vector<vk::VertexInputBindingDescription> bindingDescriptions;

        attributeDescriptions.clear();
        bindingDescriptions.clear();

        attributeDescriptions.reserve(vertexDescriptions.size());
        bindingDescriptions.reserve(vertexDescriptions.size());

        for (const auto &vertexDescription : vertexDescriptions)
        {
            const uint32_t binding = static_cast<uint32_t>(bindingDescriptions.size());

            uint32_t offset = 0;
            for (uint32_t i = 0; i < vertexDescription.attributes.size(); ++i)
            {
                const vk::Format format = vertexDescription.attributes[i];
                attributeDescriptions.emplace_back(i, binding, format, offset);
                offset += VulkanHelpers::GetFormatSize(format);
            }

            const uint32_t stride = offset;

            bindingDescriptions.emplace_back(binding, stride, vertexDescription.inputRate);
        }

        const vk::PipelineVertexInputStateCreateInfo createInfo({},
                static_cast<uint32_t>(bindingDescriptions.size()), bindingDescriptions.data(),
                static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

        return createInfo;
    }

    vk::PipelineInputAssemblyStateCreateInfo ObtainInputAssemblyStateCreateInfo(
            vk::PrimitiveTopology topology)
    {
        const vk::PipelineInputAssemblyStateCreateInfo createInfo({}, topology, false);

        return createInfo;
    }

    vk::PipelineViewportStateCreateInfo ObtainViewportStateCreateInfo(
            const vk::Extent2D &extent)
    {
        static vk::Viewport viewport;
        static vk::Rect2D scissor;

        viewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f,
                1.0f);
        scissor = vk::Rect2D(vk::Offset2D(0, 0), extent);

        const vk::PipelineViewportStateCreateInfo createInfo({}, 1, &viewport, 1, &scissor);

        return createInfo;
    }

    vk::PipelineRasterizationStateCreateInfo ObtainRasterizationStateCreateInfo(
            vk::PolygonMode polygonMode)
    {
        const vk::PipelineRasterizationStateCreateInfo createInfo({}, false, false, polygonMode,
                vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);

        return createInfo;
    }

    vk::PipelineMultisampleStateCreateInfo ObtainMultisampleStateCreateInfo(
            vk::SampleCountFlagBits sampleCount)
    {
        const vk::PipelineMultisampleStateCreateInfo createInfo({}, sampleCount, false, 0.0f, nullptr, false, false);

        return createInfo;
    }

    vk::PipelineDepthStencilStateCreateInfo ObtainDepthStencilStateCreateInfo(
            std::optional<vk::CompareOp> depthTest)
    {
        const vk::PipelineDepthStencilStateCreateInfo createInfo({},
                depthTest.has_value(), depthTest.has_value(),
                depthTest.value_or(vk::CompareOp()),
                false, false, {}, {}, 0.0f, 1.0f);

        return createInfo;
    }

    vk::PipelineColorBlendStateCreateInfo ObtainColorBlendStateCreateInfo(
            const std::vector<vk::PipelineColorBlendAttachmentState> &blendState)
    {
        vk::PipelineColorBlendStateCreateInfo createInfo({}, false, {},
                static_cast<uint32_t>(blendState.size()), blendState.data());

        return createInfo;
    }

    vk::PipelineLayout CreatePipelineLayout(vk::Device device,
            const std::vector<vk::DescriptorSetLayout> &layouts,
            const std::vector<vk::PushConstantRange> &pushConstantRanges)
    {
        const vk::PipelineLayoutCreateInfo createInfo({},
                static_cast<uint32_t>(layouts.size()), layouts.data(),
                static_cast<uint32_t>(pushConstantRanges.size()), pushConstantRanges.data());

        const auto [result, layout] = device.createPipelineLayout(createInfo);
        Assert(result == vk::Result::eSuccess);

        return layout;
    }
}

std::unique_ptr<GraphicsPipeline> GraphicsPipeline::Create(std::shared_ptr<Device> device,
        vk::RenderPass renderPass, const GraphicsPipelineProperties &properties)
{
    using namespace SGraphicsPipeline;

    const auto shaderStages = ObtainShaderStagesCreateInfo(properties.shaderModules);
    const auto vertexInputState = ObtainVertexInputStateCreateInfo(properties.vertexDescriptions);
    const auto inputAssemblyState = ObtainInputAssemblyStateCreateInfo(properties.topology);
    const auto viewportState = ObtainViewportStateCreateInfo(properties.extent);
    const auto rasterizationState = ObtainRasterizationStateCreateInfo(properties.polygonMode);
    const auto multisampleState = ObtainMultisampleStateCreateInfo(properties.sampleCount);
    const auto depthStencilState = ObtainDepthStencilStateCreateInfo(properties.depthTest);
    const auto colorBlendState = ObtainColorBlendStateCreateInfo(properties.blendState);

    vk::PipelineLayout layout = CreatePipelineLayout(device->Get(),
            properties.layouts, properties.pushConstantRanges);

    vk::GraphicsPipelineCreateInfo createInfo({},
            static_cast<uint32_t>(shaderStages.size()), shaderStages.data(),
            &vertexInputState, &inputAssemblyState, nullptr,
            &viewportState, &rasterizationState, &multisampleState,
            &depthStencilState, &colorBlendState, nullptr,
            layout, renderPass, 0, nullptr, 0);

    const auto [result, pipeline] = device->Get().createGraphicsPipeline(nullptr, createInfo);
    Assert(result == vk::Result::eSuccess);

    return std::make_unique<GraphicsPipeline>(device, pipeline, layout);
}

GraphicsPipeline::GraphicsPipeline(std::shared_ptr<Device> aDevice, vk::Pipeline aPipeline, vk::PipelineLayout aLayout)
    : device(aDevice)
    , pipeline(aPipeline)
    , layout(aLayout)
{}

GraphicsPipeline::~GraphicsPipeline()
{
    device->Get().destroyPipelineLayout(layout);
    device->Get().destroyPipeline(pipeline);
}
