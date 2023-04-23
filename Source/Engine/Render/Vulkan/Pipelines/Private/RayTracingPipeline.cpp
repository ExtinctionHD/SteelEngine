#include "Engine/Render/Vulkan/Pipelines/RayTracingPipeline.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    static vk::RayTracingShaderGroupTypeKHR GetShaderGroupType(
            ShaderGroupType type, const ShaderGroup& shaderGroup)
    {
        switch (type)
        {
        case ShaderGroupType::eRaygen:
            Assert(shaderGroup.generalShader != VK_SHADER_UNUSED_KHR);
            return vk::RayTracingShaderGroupTypeKHR::eGeneral;
        case ShaderGroupType::eMiss:
            Assert(shaderGroup.generalShader != VK_SHADER_UNUSED_KHR);
            return vk::RayTracingShaderGroupTypeKHR::eGeneral;
        case ShaderGroupType::eHit:
            if (shaderGroup.intersectionShader != VK_SHADER_UNUSED_KHR)
            {
                return vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup;
            }
            return vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
        default:
            Assert(false);
            return {};
        }
    }

    static std::vector<vk::RayTracingShaderGroupCreateInfoKHR> CreateShaderGroupsCreateInfo(
            const std::map<ShaderGroupType, std::vector<ShaderGroup>>& shaderGroupsMap)
    {
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> createInfo;
        createInfo.reserve(shaderGroupsMap.size());

        for (const auto& [type, shaderGroups] : shaderGroupsMap)
        {
            for (const auto& shaderGroup : shaderGroups)
            {
                createInfo.emplace_back(GetShaderGroupType(type, shaderGroup), shaderGroup.generalShader,
                        shaderGroup.closestHitShader, shaderGroup.anyHitShader, shaderGroup.intersectionShader);
            }
        }

        return createInfo;
    }

    static Bytes RealignShaderGroupsData(const Bytes& source,
            uint32_t handleSize, uint32_t baseAlignment)
    {
        Assert(source.size() % handleSize == 0);
        Assert(baseAlignment % handleSize == 0);

        const uint32_t multiplier = baseAlignment / handleSize;

        Bytes result(source.size() * multiplier);

        for (size_t i = 0; i < source.size() / handleSize; ++i)
        {
            std::memcpy(result.data() + i * baseAlignment, source.data() + i * handleSize, handleSize);
        }

        return result;
    }

    static vk::Buffer CreateShaderGroupsBuffer(vk::Pipeline pipeline, uint32_t groupCount)
    {
        const uint32_t handleSize = VulkanContext::device->GetRayTracingProperties().shaderGroupHandleSize;
        const uint32_t baseAlignment = VulkanContext::device->GetRayTracingProperties().shaderGroupBaseAlignment;

        const uint32_t shaderGroupsDataSize = handleSize * groupCount;
        auto [result, shaderGroupsData] = VulkanContext::device->Get().getRayTracingShaderGroupHandlesKHR<uint8_t>(
                pipeline, 0, groupCount, shaderGroupsDataSize);

        Assert(result == vk::Result::eSuccess);

        shaderGroupsData = RealignShaderGroupsData(shaderGroupsData, handleSize, baseAlignment);

        const vk::BufferUsageFlags bufferUsage
                = vk::BufferUsageFlagBits::eShaderBindingTableKHR
                | vk::BufferUsageFlagBits::eShaderDeviceAddress;

        return BufferHelpers::CreateBufferWithData(bufferUsage, ByteView(shaderGroupsData));
    }

    static ShaderBindingTable GenerateSBT(vk::Pipeline pipeline,
            const std::map<ShaderGroupType, std::vector<ShaderGroup>>& shaderGroupsMap)
    {
        const uint32_t baseAlignment = VulkanContext::device->GetRayTracingProperties().shaderGroupBaseAlignment;

        uint32_t groupCount = 0;
        std::map<ShaderGroupType, uint32_t> offsets;
        for (const auto& [type, shaderGroups] : shaderGroupsMap)
        {
            offsets[type] = groupCount * baseAlignment;

            groupCount += static_cast<uint32_t>(shaderGroups.size());
        }

        Assert(offsets.contains(ShaderGroupType::eRaygen));
        Assert(offsets.contains(ShaderGroupType::eMiss));
        Assert(offsets.contains(ShaderGroupType::eHit));

        return ShaderBindingTable{
            CreateShaderGroupsBuffer(pipeline, groupCount),
            offsets.at(ShaderGroupType::eRaygen),
            offsets.at(ShaderGroupType::eMiss),
            offsets.at(ShaderGroupType::eHit),
            baseAlignment
        };
    }
}

std::unique_ptr<RayTracingPipeline> RayTracingPipeline::Create(const Description& description)
{
    const auto shaderStagesCreateInfo = ShaderHelpers::CreateShaderStagesCreateInfo(description.shaderModules);
    const auto shaderGroupsCreateInfo = Details::CreateShaderGroupsCreateInfo(description.shaderGroups);

    const ShaderReflection reflection = ShaderHelpers::MergeShaderReflections(description.shaderModules);

    const std::vector<vk::DescriptorSetLayout> descriptorSetLayouts
            = ShaderHelpers::CreateDescriptorSetLayouts(reflection);

    const std::vector<vk::PushConstantRange> pushConstantRanges
            = ShaderHelpers::GetPushConstantRanges(reflection);

    const vk::PipelineLayout layout = VulkanHelpers::CreatePipelineLayout(
            VulkanContext::device->Get(), descriptorSetLayouts, pushConstantRanges);

    const vk::RayTracingPipelineCreateInfoKHR createInfo({},
            static_cast<uint32_t>(shaderStagesCreateInfo.size()), shaderStagesCreateInfo.data(),
            static_cast<uint32_t>(shaderGroupsCreateInfo.size()), shaderGroupsCreateInfo.data(),
            8, nullptr, nullptr, nullptr, layout);

    const auto [result, pipeline] = VulkanContext::device->Get().createRayTracingPipelineKHR(
            vk::DeferredOperationKHR(), vk::PipelineCache(), createInfo);

    Assert(result == vk::Result::eSuccess);

    const ShaderBindingTable shaderBindingTable = Details::GenerateSBT(pipeline, description.shaderGroups);

    return std::unique_ptr<RayTracingPipeline>(new RayTracingPipeline(pipeline, layout,
            descriptorSetLayouts, reflection.pushConstants, shaderBindingTable));
}

RayTracingPipeline::RayTracingPipeline(vk::Pipeline pipeline_, vk::PipelineLayout layout_,
        const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts_,
        const std::map<std::string, vk::PushConstantRange>& pushConstants_,
        const ShaderBindingTable& shaderBindingTable_)
    : PipelineBase(pipeline_, layout_, descriptorSetLayouts_, pushConstants_)
    , shaderBindingTable(shaderBindingTable_)
{}
