#pragma once

#include "Engine/Render/Vulkan/Resources/DescriptorHelpers.hpp"

#include "Utils/DataHelpers.hpp"

struct ShaderSpecialization
{
    ShaderSpecialization() = default;
    ~ShaderSpecialization() = default;
    ShaderSpecialization(const ShaderSpecialization& other);
    ShaderSpecialization(ShaderSpecialization&& other) noexcept;
    ShaderSpecialization& operator=(ShaderSpecialization other);

    std::vector<vk::SpecializationMapEntry> map;
    vk::SpecializationInfo info;
    Bytes data;
};

using DescriptorsReflection = std::map<std::string, DescriptorDescription>;
using PushConstantsReflection = std::map<std::string, vk::PushConstantRange>;

struct ShaderReflection
{
    DescriptorsReflection descriptors;
    PushConstantsReflection pushConstants;
};

struct ShaderModule
{
    vk::ShaderModule module;
    vk::ShaderStageFlagBits stage;
    ShaderSpecialization specialization;
    ShaderReflection reflection;
};

namespace ShaderHelpers
{
    std::vector<vk::PipelineShaderStageCreateInfo> CreateShaderStagesCreateInfo(
        const std::vector<ShaderModule>& shaderModules);

    ShaderReflection RetrieveShaderReflection(const std::vector<uint32_t>& spirvCode);

    ShaderReflection MergeShaderReflections(const std::vector<ShaderReflection>& reflections);

    ShaderReflection MergeShaderReflections(const std::vector<ShaderModule>& shaderModules);

    std::vector<vk::DescriptorSetLayout> GetDescriptorSetLayouts(const DescriptorsReflection& reflection);

    std::vector<vk::PushConstantRange> GetPushConstantRanges(const PushConstantsReflection& reflection);

    template <class... Types>
    ShaderSpecialization BuildShaderSpecialization(const std::tuple<Types...>& specializationValues)
    {
        constexpr uint32_t valueCount = static_cast<uint32_t>(std::tuple_size_v<std::tuple<Types...>>);

        ShaderSpecialization specialization;

        uint32_t i = 0;
        uint32_t offset = 0;

        const auto func = [&](const auto& value)
        {
            const uint32_t size = static_cast<uint32_t>(sizeof(value));

            specialization.map.emplace_back(i++, offset, size);

            specialization.data.resize(offset + size);
            std::memcpy(specialization.data.data() + offset, &value, size);

            offset += size;
        };

        std::apply([&](auto const&... values)
            { (func(values), ...); },
            specializationValues);

        specialization.info = vk::SpecializationInfo(
            valueCount, specialization.map.data(), offset, specialization.data.data());

        return specialization;
    }
}
