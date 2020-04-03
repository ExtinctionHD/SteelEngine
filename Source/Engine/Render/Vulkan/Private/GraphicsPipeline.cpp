#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"

#include "Utils/Assert.hpp"

namespace SGraphicsPipeline
{
    vk::PipelineVertexInputStateCreateInfo BuildVertexInputStateCreateInfo(
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
            for (uint32_t i = 0; i < vertexDescription.format.size(); ++i)
            {
                const vk::Format format = vertexDescription.format[i];
                attributeDescriptions.emplace_back(i, binding, format, offset);
                offset += ImageHelpers::GetTexelSize(format);
            }

            const uint32_t stride = offset;

            bindingDescriptions.emplace_back(binding, stride, vertexDescription.inputRate);
        }

        const vk::PipelineVertexInputStateCreateInfo createInfo({},
                static_cast<uint32_t>(bindingDescriptions.size()), bindingDescriptions.data(),
                static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

        return createInfo;
    }

    vk::PipelineInputAssemblyStateCreateInfo BuildInputAssemblyStateCreateInfo(
            vk::PrimitiveTopology topology)
    {
        const vk::PipelineInputAssemblyStateCreateInfo createInfo({}, topology, false);

        return createInfo;
    }

    vk::PipelineViewportStateCreateInfo BuildViewportStateCreateInfo(
            const vk::Extent2D &extent)
    {
        static vk::Viewport viewport;
        static vk::Rect2D scissor;

        viewport = vk::Viewport(0.0f, 0.0f,
                static_cast<float>(extent.width),
                static_cast<float>(extent.height),
                0.0f, 1.0f);

        scissor = vk::Rect2D(vk::Offset2D(0, 0), extent);

        const vk::PipelineViewportStateCreateInfo createInfo({}, 1, &viewport, 1, &scissor);

        return createInfo;
    }

    vk::PipelineRasterizationStateCreateInfo BuildRasterizationStateCreateInfo(
            vk::PolygonMode polygonMode, vk::CullModeFlagBits cullMode, vk::FrontFace frontFace)
    {
        const vk::PipelineRasterizationStateCreateInfo createInfo({}, false, false, polygonMode,
                cullMode, frontFace, false, 0.0f, 0.0f, 0.0f, 1.0f);

        return createInfo;
    }

    vk::PipelineMultisampleStateCreateInfo BuildMultisampleStateCreateInfo(
            vk::SampleCountFlagBits sampleCount)
    {
        const vk::PipelineMultisampleStateCreateInfo createInfo({}, sampleCount, false, 0.0f, nullptr, false, false);

        return createInfo;
    }

    vk::PipelineDepthStencilStateCreateInfo BuildDepthStencilStateCreateInfo(
            std::optional<vk::CompareOp> depthTest)
    {
        const vk::PipelineDepthStencilStateCreateInfo createInfo({},
                depthTest.has_value(), depthTest.has_value(), depthTest.value_or(vk::CompareOp()),
                false, false, vk::StencilOpState(), vk::StencilOpState(), 0.0f, 1.0f);

        return createInfo;
    }

    vk::PipelineColorBlendStateCreateInfo BuildColorBlendStateCreateInfo(
            const std::vector<BlendMode> &blendModes)
    {
        static std::vector<vk::PipelineColorBlendAttachmentState> blendStates;

        blendStates.clear();
        blendStates.reserve(blendModes.size());

        for (const auto &blendMode : blendModes)
        {
            switch (blendMode)
            {
            case BlendMode::eDisabled:
                blendStates.emplace_back(false,
                        vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                        vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                        ImageHelpers::kColorComponentFlagsRGBA);
                break;
            case BlendMode::eAlphaBlend:
                blendStates.emplace_back(true,
                        vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
                        vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                        ImageHelpers::kColorComponentFlagsRGBA);
                break;
            default:
                Assert(false);
                break;
            }
        }

        vk::PipelineColorBlendStateCreateInfo createInfo({}, false, {},
                static_cast<uint32_t>(blendStates.size()), blendStates.data());

        return createInfo;
    }
}

std::unique_ptr<GraphicsPipeline> GraphicsPipeline::Create(vk::RenderPass renderPass,
        const GraphicsPipelineDescription &description)
{
    using namespace SGraphicsPipeline;

    const auto shaderStages = ShaderHelpers::BuildShaderStagesCreateInfo(description.shaderModules);
    const auto vertexInputState = BuildVertexInputStateCreateInfo(description.vertexDescriptions);
    const auto inputAssemblyState = BuildInputAssemblyStateCreateInfo(description.topology);
    const auto viewportState = BuildViewportStateCreateInfo(description.extent);
    const auto rasterizationState = BuildRasterizationStateCreateInfo(description.polygonMode,
            description.cullMode, description.frontFace);
    const auto multisampleState = BuildMultisampleStateCreateInfo(description.sampleCount);
    const auto depthStencilState = BuildDepthStencilStateCreateInfo(description.depthTest);
    const auto colorBlendState = BuildColorBlendStateCreateInfo(description.attachmentsBlendModes);

    const vk::PipelineLayout layout = VulkanHelpers::CreatePipelineLayout(VulkanContext::device->Get(),
            description.layouts, description.pushConstantRanges);

    const vk::GraphicsPipelineCreateInfo createInfo({},
            static_cast<uint32_t>(shaderStages.size()), shaderStages.data(),
            &vertexInputState, &inputAssemblyState, nullptr,
            &viewportState, &rasterizationState, &multisampleState,
            &depthStencilState, &colorBlendState, nullptr,
            layout, renderPass, 0, nullptr, 0);

    const auto [result, pipeline] = VulkanContext::device->Get().createGraphicsPipeline(nullptr, createInfo);
    Assert(result == vk::Result::eSuccess);

    return std::unique_ptr<GraphicsPipeline>(new GraphicsPipeline(pipeline, layout));
}

GraphicsPipeline::GraphicsPipeline(vk::Pipeline pipeline_, vk::PipelineLayout layout_)
    : pipeline(pipeline_)
    , layout(layout_)
{}

GraphicsPipeline::~GraphicsPipeline()
{
    VulkanContext::device->Get().destroyPipelineLayout(layout);
    VulkanContext::device->Get().destroyPipeline(pipeline);
}
