#include <spirv_reflect.h>

#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"

#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace Details
{
    static vk::DescriptorType GetDescriptorType(SpvReflectDescriptorType descriptorType)
    {
        switch (descriptorType)
        {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            return vk::DescriptorType::eSampler;
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            return vk::DescriptorType::eCombinedImageSampler;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return vk::DescriptorType::eSampledImage;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return vk::DescriptorType::eStorageImage;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return vk::DescriptorType::eUniformTexelBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return vk::DescriptorType::eStorageTexelBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return vk::DescriptorType::eUniformBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return vk::DescriptorType::eStorageBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return vk::DescriptorType::eUniformBufferDynamic;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return vk::DescriptorType::eStorageBufferDynamic;
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return vk::DescriptorType::eInputAttachment;
        case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
            return vk::DescriptorType::eAccelerationStructureKHR;
        default:
            Assert(false);
            return {};
        }
    }

    static vk::ShaderStageFlagBits GetShaderStage(SpvReflectShaderStageFlagBits shaderStage)
    {
        switch (shaderStage)
        {
        case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
            return vk::ShaderStageFlagBits::eVertex;
        case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            return vk::ShaderStageFlagBits::eTessellationControl;
        case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            return vk::ShaderStageFlagBits::eTessellationEvaluation;
        case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
            return vk::ShaderStageFlagBits::eGeometry;
        case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
            return vk::ShaderStageFlagBits::eFragment;
        case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
            return vk::ShaderStageFlagBits::eCompute;
        case SPV_REFLECT_SHADER_STAGE_TASK_BIT_EXT:
            return vk::ShaderStageFlagBits::eTaskEXT;
        case SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT:
            return vk::ShaderStageFlagBits::eMeshEXT;
        case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR:
            return vk::ShaderStageFlagBits::eRaygenKHR;
        case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR:
            return vk::ShaderStageFlagBits::eAnyHitKHR;
        case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
            return vk::ShaderStageFlagBits::eClosestHitKHR;
        case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR:
            return vk::ShaderStageFlagBits::eMissKHR;
        case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR:
            return vk::ShaderStageFlagBits::eIntersectionKHR;
        case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR:
            return vk::ShaderStageFlagBits::eCallableKHR;
        default:
            Assert(false);
            return {};
        }
    }

    static std::string GetDescriptorName(const SpvReflectDescriptorBinding& descriptorBinding)
    {
        if (*descriptorBinding.name != 0)
        {
            return descriptorBinding.name;
        }

        Assert(descriptorBinding.type_description->member_count == 1);
        Assert(descriptorBinding.type_description->members[0].struct_member_name);
        Assert(descriptorBinding.type_description->members[0].struct_member_name);

        return descriptorBinding.type_description->members[0].struct_member_name;
    }

    static DescriptorDescription BuildDescriptorDescription(
        const SpvReflectDescriptorBinding& descriptorBinding)
    {
        vk::DescriptorBindingFlags bindingFlags;

        if (descriptorBinding.count > 1)
        {
            bindingFlags |= vk::DescriptorBindingFlagBits::ePartiallyBound;
            bindingFlags |= vk::DescriptorBindingFlagBits::eUpdateAfterBind;
        }

        if (descriptorBinding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
        {
            bindingFlags |= vk::DescriptorBindingFlagBits::eUpdateAfterBind;
        }

        return DescriptorDescription{ DescriptorKey{ descriptorBinding.set, descriptorBinding.binding },
            descriptorBinding.count,
            GetDescriptorType(descriptorBinding.descriptor_type),
            vk::ShaderStageFlags(),
            bindingFlags };
    }

    static DescriptorsReflection BuildDescriptorsReflection(const spv_reflect::ShaderModule& shaderModule)
    {
        uint32_t descriptorSetCount;
        SpvReflectResult result = shaderModule.EnumerateDescriptorSets(&descriptorSetCount, nullptr);
        Assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectDescriptorSet*> descriptorSets(descriptorSetCount);
        result = shaderModule.EnumerateDescriptorSets(&descriptorSetCount, descriptorSets.data());
        Assert(result == SPV_REFLECT_RESULT_SUCCESS);

        DescriptorsReflection descriptorsReflection;

        for (const auto& descriptorSet : descriptorSets)
        {
            for (uint32_t bindingIndex = 0; bindingIndex < descriptorSet->binding_count; ++bindingIndex)
            {
                const SpvReflectDescriptorBinding& descriptorBinding = *descriptorSet->bindings[bindingIndex];

                descriptorsReflection.emplace(
                    GetDescriptorName(descriptorBinding), BuildDescriptorDescription(descriptorBinding));
            }
        }

        const vk::ShaderStageFlagBits shaderStage = GetShaderStage(shaderModule.GetShaderStage());

        for (auto& [name, descriptorDescription] : descriptorsReflection)
        {
            descriptorDescription.stageFlags = shaderStage;
        }

        return descriptorsReflection;
    }

    static PushConstantsReflection BuildPushConstantsReflection(const spv_reflect::ShaderModule& shaderModule)
    {
        uint32_t pushConstantCount;
        SpvReflectResult result = shaderModule.EnumeratePushConstantBlocks(&pushConstantCount, nullptr);
        Assert(result == SPV_REFLECT_RESULT_SUCCESS);

        std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
        result = shaderModule.EnumeratePushConstantBlocks(&pushConstantCount, pushConstants.data());
        Assert(result == SPV_REFLECT_RESULT_SUCCESS);

        const vk::ShaderStageFlagBits shaderStage = GetShaderStage(shaderModule.GetShaderStage());

        PushConstantsReflection pushConstantsReflection;

        for (const auto pushConstant : pushConstants)
        {
            for (uint32_t i = 0; i < pushConstant->member_count; ++i)
            {
                const SpvReflectBlockVariable& member = pushConstant->members[i];

                const vk::PushConstantRange pushConstantRange(shaderStage, member.offset, member.size);

                pushConstantsReflection.emplace(std::string(member.name), pushConstantRange);
            }
        }

        return pushConstantsReflection;
    }

    static void MergeDescriptorsReflections(
        DescriptorsReflection& dstDescriptors, const DescriptorsReflection& srcDescriptors)
    {
        for (const auto& [name, srcDescriptorDescription] : srcDescriptors)
        {
            const auto it = dstDescriptors.find(name);

            if (it != dstDescriptors.end())
            {
                DescriptorDescription& dstDescriptorDescription = it->second;

                Assert(dstDescriptorDescription.key == srcDescriptorDescription.key);
                Assert(dstDescriptorDescription.count == srcDescriptorDescription.count);
                Assert(dstDescriptorDescription.type == srcDescriptorDescription.type);
                Assert(dstDescriptorDescription.bindingFlags == srcDescriptorDescription.bindingFlags);

                dstDescriptorDescription.stageFlags |= srcDescriptorDescription.stageFlags;
            }
            else
            {
                dstDescriptors.emplace(name, srcDescriptorDescription);
            }
        }
    }

    static void MergePushConstantsReflections(
        PushConstantsReflection& dstPushConstants, const PushConstantsReflection& srcPushConstants)
    {
        std::vector<std::string> newNames;

        for (const auto& [name, pushConstantRange] : srcPushConstants)
        {
            if (auto it = dstPushConstants.find(name); it != dstPushConstants.end())
            {
                Assert(it->second.offset == pushConstantRange.offset);
                Assert(it->second.size == pushConstantRange.size);

                it->second.stageFlags |= pushConstantRange.stageFlags;
            }
            else
            {
                newNames.push_back(name);
            }
        }

        for (const auto& name : newNames)
        {
            dstPushConstants.emplace(name, srcPushConstants.at(name));
        }
    }
}

ShaderSpecialization::ShaderSpecialization(const ShaderSpecialization& other)
    : map(other.map), data(other.data)
{
    info = other.info;
    info.pMapEntries = map.data();
    info.pData = data.data();
}

ShaderSpecialization::ShaderSpecialization(ShaderSpecialization&& other) noexcept
    : map(std::move(other.map)), data(std::move(other.data))
{
    info = other.info;
    info.pMapEntries = map.data();
    info.pData = data.data();
}

ShaderSpecialization& ShaderSpecialization::operator=(ShaderSpecialization other)
{
    if (this != &other)
    {
        std::swap(map, other.map);
        std::swap(info, other.info);
        std::swap(data, other.data);
    }

    return *this;
}

std::vector<vk::PipelineShaderStageCreateInfo> ShaderHelpers::CreateShaderStagesCreateInfo(
    const std::vector<ShaderModule>& shaderModules)
{
    std::vector<vk::PipelineShaderStageCreateInfo> createInfo;
    createInfo.reserve(shaderModules.size());

    for (const auto& shaderModule : shaderModules)
    {
        const vk::SpecializationInfo* pSpecializationInfo = nullptr;

        if (!shaderModule.specialization.map.empty())
        {
            pSpecializationInfo = &shaderModule.specialization.info;
        }

        createInfo.emplace_back(vk::PipelineShaderStageCreateFlags(), shaderModule.stage, shaderModule.module, "main", pSpecializationInfo);
    }

    return createInfo;
}

ShaderReflection ShaderHelpers::RetrieveShaderReflection(const std::vector<uint32_t>& spirvCode)
{
    const spv_reflect::ShaderModule shaderModule(spirvCode);

    ShaderReflection reflection;
    reflection.descriptors = Details::BuildDescriptorsReflection(shaderModule);
    reflection.pushConstants = Details::BuildPushConstantsReflection(shaderModule);

    return reflection;
}

ShaderReflection ShaderHelpers::MergeShaderReflections(const std::vector<ShaderReflection>& reflections)
{
    ShaderReflection mergedReflection;

    for (const auto& reflection : reflections)
    {
        Details::MergeDescriptorsReflections(mergedReflection.descriptors, reflection.descriptors);
        Details::MergePushConstantsReflections(mergedReflection.pushConstants, reflection.pushConstants);
    }

    return mergedReflection;
}

ShaderReflection ShaderHelpers::MergeShaderReflections(const std::vector<ShaderModule>& shaderModules)
{
    ShaderReflection mergedReflection;

    for (const auto& shaderModule : shaderModules)
    {
        Details::MergeDescriptorsReflections(
            mergedReflection.descriptors, shaderModule.reflection.descriptors);
        Details::MergePushConstantsReflections(
            mergedReflection.pushConstants, shaderModule.reflection.pushConstants);
    }

    return mergedReflection;
}

std::vector<vk::DescriptorSetLayout> ShaderHelpers::GetDescriptorSetLayouts(
    const DescriptorsReflection& reflection)
{
    std::map<uint32_t, DescriptorSetDescription> descriptionMap;

    for (const auto& [name, descriptorDescription] : reflection)
    {
        descriptionMap[descriptorDescription.key.set].push_back(descriptorDescription);
    }

    std::vector<vk::DescriptorSetLayout> layouts(descriptionMap.size());

    for (uint32_t i = 0; i < layouts.size(); ++i)
    {
        Assert(descriptionMap.contains(i));

        layouts[i] = VulkanContext::descriptorManager->GetDescriptorSetLayout(descriptionMap[i]);
    }

    return layouts;
}

std::vector<vk::PushConstantRange> ShaderHelpers::GetPushConstantRanges(
    const PushConstantsReflection& reflection)
{
    std::vector<vk::PushConstantRange> pushConstantRanges;

    for (const auto& [name, range] : reflection)
    {
        const auto rangeLocal = range;
        const auto pred = [&](const vk::PushConstantRange& pushConstantRange)
        { return pushConstantRange.stageFlags == rangeLocal.stageFlags; };

        auto it = std::ranges::find_if(pushConstantRanges, pred);

        if (it != pushConstantRanges.end())
        {
            if (it->offset < range.offset)
            {
                Assert(it->offset + it->size <= range.offset);

                it->size = range.offset + range.size - it->offset;
            }
            else
            {
                Assert(range.offset + range.size <= it->offset);

                it->size = it->offset + it->size - range.offset;

                it->offset = range.offset;
            }
        }
        else
        {
            pushConstantRanges.push_back(range);
        }
    }

    return pushConstantRanges;
}
