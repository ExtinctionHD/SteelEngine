#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    static constexpr std::array<vk::DynamicState, 2> kDynamicStates{
        vk::DynamicState::eViewport, vk::DynamicState::eScissor
    };

    static vk::CompareOp ReverseCompareOp(vk::CompareOp compareOp)
    {
        switch (compareOp)
        {
        case vk::CompareOp::eNever:
            return vk::CompareOp::eAlways;
        case vk::CompareOp::eLess:
            return vk::CompareOp::eGreater;
        case vk::CompareOp::eEqual:
            return vk::CompareOp::eNotEqual;
        case vk::CompareOp::eLessOrEqual:
            return vk::CompareOp::eGreaterOrEqual;
        case vk::CompareOp::eGreater:
            return vk::CompareOp::eLess;
        case vk::CompareOp::eNotEqual:
            return vk::CompareOp::eEqual;
        case vk::CompareOp::eGreaterOrEqual:
            return vk::CompareOp::eLessOrEqual;
        case vk::CompareOp::eAlways:
            return vk::CompareOp::eEqual;
        default:
            Assert(false);
            return compareOp;
        }
    }

    static vk::PipelineVertexInputStateCreateInfo CreateVertexInputStateCreateInfo(
            const std::vector<VertexDescription>& vertexDescriptions)
    {
        static std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
        static std::vector<vk::VertexInputBindingDescription> bindingDescriptions;

        attributeDescriptions.clear();
        bindingDescriptions.clear();

        attributeDescriptions.reserve(vertexDescriptions.size());
        bindingDescriptions.reserve(vertexDescriptions.size());

        uint32_t location = 0;
        for (const auto& vertexDescription : vertexDescriptions)
        {
            const uint32_t binding = static_cast<uint32_t>(bindingDescriptions.size());

            uint32_t offset = 0;
            for (const auto& format : vertexDescription.format)
            {
                attributeDescriptions.emplace_back(location++, binding, format, offset);
                offset += ImageHelpers::GetTexelSize(format);
            }

            uint32_t stride = offset;
            if (vertexDescription.stride > 0)
            {
                Assert(vertexDescription.stride >= offset);
                stride = vertexDescription.stride;
            }

            bindingDescriptions.emplace_back(binding, stride, vertexDescription.inputRate);
        }

        const vk::PipelineVertexInputStateCreateInfo createInfo({}, bindingDescriptions, attributeDescriptions);

        return createInfo;
    }

    static vk::PipelineInputAssemblyStateCreateInfo CreateInputAssemblyStateCreateInfo(
            vk::PrimitiveTopology topology)
    {
        const vk::PipelineInputAssemblyStateCreateInfo createInfo({}, topology, false);

        return createInfo;
    }

    static vk::PipelineViewportStateCreateInfo CreateViewportStateCreateInfo()
    {
        const vk::PipelineViewportStateCreateInfo createInfo({}, 1, nullptr, 1, nullptr);

        return createInfo;
    }

    static vk::PipelineRasterizationStateCreateInfo CreateRasterizationStateCreateInfo(
            vk::PolygonMode polygonMode, vk::CullModeFlagBits cullMode, vk::FrontFace frontFace)
    {
        const vk::PipelineRasterizationStateCreateInfo createInfo({}, false, false, polygonMode,
                cullMode, frontFace, false, 0.0f, 0.0f, 0.0f, 1.0f);

        return createInfo;
    }

    static vk::PipelineMultisampleStateCreateInfo CreateMultisampleStateCreateInfo(
            vk::SampleCountFlagBits sampleCount)
    {
        const vk::PipelineMultisampleStateCreateInfo createInfo({}, sampleCount, false, 0.0f, nullptr, false, false);

        return createInfo;
    }

    static vk::PipelineDepthStencilStateCreateInfo CreateDepthStencilStateCreateInfo(
            std::optional<vk::CompareOp> depthTest)
    {
        vk::CompareOp compareOp = depthTest.value_or(vk::CompareOp());
        compareOp = Config::kReverseDepth ? ReverseCompareOp(compareOp) : compareOp;

        const vk::PipelineDepthStencilStateCreateInfo createInfo({},
                depthTest.has_value(), depthTest.has_value(), compareOp,
                false, false, vk::StencilOpState(), vk::StencilOpState(), 0.0f, 1.0f);

        return createInfo;
    }

    static vk::PipelineColorBlendStateCreateInfo CreateColorBlendStateCreateInfo(
            const std::vector<BlendMode>& blendModes)
    {
        static std::vector<vk::PipelineColorBlendAttachmentState> blendStates;

        blendStates.clear();
        blendStates.reserve(blendModes.size());

        for (const auto& blendMode : blendModes)
        {
            switch (blendMode)
            {
            case BlendMode::eDisabled:
                blendStates.emplace_back(false,
                        vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                        vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                        ImageHelpers::kColorComponentsRGBA);
                break;
            case BlendMode::eAlphaBlend:
                blendStates.emplace_back(true,
                        vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
                        vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                        ImageHelpers::kColorComponentsRGBA);
                break;
            default:
                Assert(false);
                break;
            }
        }

        const vk::PipelineColorBlendStateCreateInfo createInfo({}, false, vk::LogicOp(), blendStates);

        return createInfo;
    }

    static vk::PipelineDynamicStateCreateInfo CreateDynamicStateCreateInfo()
    {
        return vk::PipelineDynamicStateCreateInfo({}, kDynamicStates);
    }
}

std::unique_ptr<GraphicsPipeline> GraphicsPipeline::Create(
        vk::RenderPass renderPass, const Description& description)
{
    const std::vector<vk::PipelineShaderStageCreateInfo> shaderStages
            = ShaderHelpers::CreateShaderStagesCreateInfo(description.shaderModules);

    const vk::PipelineVertexInputStateCreateInfo vertexInputState
            = Details::CreateVertexInputStateCreateInfo(description.vertexDescriptions);

    const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState
            = Details::CreateInputAssemblyStateCreateInfo(description.topology);

    const vk::PipelineViewportStateCreateInfo viewportState
            = Details::CreateViewportStateCreateInfo();

    const vk::PipelineRasterizationStateCreateInfo rasterizationState
            = Details::CreateRasterizationStateCreateInfo(description.polygonMode,
                    description.cullMode, description.frontFace);

    const vk::PipelineMultisampleStateCreateInfo multisampleState
            = Details::CreateMultisampleStateCreateInfo(description.sampleCount);

    const vk::PipelineDepthStencilStateCreateInfo depthStencilState
            = Details::CreateDepthStencilStateCreateInfo(description.depthTest);

    const vk::PipelineColorBlendStateCreateInfo colorBlendState
            = Details::CreateColorBlendStateCreateInfo(description.attachmentsBlendModes);

    const vk::PipelineDynamicStateCreateInfo dynamicState
            = Details::CreateDynamicStateCreateInfo();

    const vk::PipelineLayout layout = VulkanHelpers::CreatePipelineLayout(VulkanContext::device->Get(),
            description.layouts, description.pushConstantRanges);

    const vk::GraphicsPipelineCreateInfo createInfo({},
            shaderStages, &vertexInputState, &inputAssemblyState, nullptr,
            &viewportState, &rasterizationState, &multisampleState,
            &depthStencilState, &colorBlendState, &dynamicState,
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
