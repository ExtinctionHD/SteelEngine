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

struct ShaderReflection
{
    std::vector<DescriptorSetDescription> descriptorSets;
    std::map<std::string, vk::PushConstantRange> pushConstants;
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

    std::vector<vk::DescriptorSetLayout> CreateDescriptorSetLayouts(const ShaderReflection& reflection);

    std::vector<vk::PushConstantRange> GetPushConstantRanges(const ShaderReflection& reflection);

    template <class... Types>
    ShaderSpecialization BuildShaderSpecialization(const std::tuple<Types...>& specializationValues)
    {
        constexpr uint32_t valueCount = static_cast<uint32_t>(std::tuple_size_v<std::tuple<Types...>>);

        ShaderSpecialization specialization;

        uint32_t i = 0;
        uint32_t offset = 0;

        const auto functor = [&](const auto& value)
            {
                const uint32_t size = static_cast<uint32_t>(sizeof(value));

                specialization.map.emplace_back(i++, offset, size);

                specialization.data.resize(offset + size);
                std::memcpy(specialization.data.data() + offset, &value, size);

                offset += size;
            };

        std::apply([&](auto const&... values) { (functor(values), ...); }, specializationValues);

        specialization.info = vk::SpecializationInfo(valueCount,
                specialization.map.data(), offset, specialization.data.data());

        return specialization;
    }
}
