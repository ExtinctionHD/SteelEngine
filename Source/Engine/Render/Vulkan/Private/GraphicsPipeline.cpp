#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Assert.hpp"

namespace SGraphicsPipeline
{
    std::vector<vk::PipelineShaderStageCreateInfo> BuildShaderStagesCreateInfo(
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
            vk::PolygonMode polygonMode)
    {
        const vk::PipelineRasterizationStateCreateInfo createInfo({}, false, false, polygonMode,
                vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);

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
                depthTest.has_value(), depthTest.has_value(),
                depthTest.value_or(vk::CompareOp()),
                false, false, {}, {}, 0.0f, 1.0f);

        return createInfo;
    }

    vk::PipelineColorBlendStateCreateInfo BuildColorBlendStateCreateInfo(
            const std::vector<eBlendMode> &blendModes)
    {
        static std::vector<vk::PipelineColorBlendAttachmentState> blendStates;

        blendStates.clear();
        blendStates.reserve(blendModes.size());

        for (const auto &blendMode : blendModes)
        {
            switch (blendMode)
            {
            case eBlendMode::kDisabled:
                blendStates.emplace_back(false,
                        vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                        vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                        VulkanHelpers::kColorComponentFlagsRgba);
                break;
            case eBlendMode::kAlphaBlend:
                blendStates.emplace_back(true,
                        vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
                        vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                        VulkanHelpers::kColorComponentFlagsRgba);
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
        vk::RenderPass renderPass, const GraphicsPipelineDescription &description)
{
    using namespace SGraphicsPipeline;

    const auto shaderStages = BuildShaderStagesCreateInfo(description.shaderModules);
    const auto vertexInputState = BuildVertexInputStateCreateInfo(description.vertexDescriptions);
    const auto inputAssemblyState = BuildInputAssemblyStateCreateInfo(description.topology);
    const auto viewportState = BuildViewportStateCreateInfo(description.extent);
    const auto rasterizationState = BuildRasterizationStateCreateInfo(description.polygonMode);
    const auto multisampleState = BuildMultisampleStateCreateInfo(description.sampleCount);
    const auto depthStencilState = BuildDepthStencilStateCreateInfo(description.depthTest);
    const auto colorBlendState = BuildColorBlendStateCreateInfo(description.attachmentsBlendModes);

    vk::PipelineLayout layout = CreatePipelineLayout(device->Get(),
            description.layouts, description.pushConstantRanges);

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
