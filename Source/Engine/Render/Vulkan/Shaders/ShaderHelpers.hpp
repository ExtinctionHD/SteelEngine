#pragma once

#include "Utils/DataHelpers.hpp"

struct ShaderSpecialization
{
    ShaderSpecialization() = default;
    ~ShaderSpecialization() = default;
    ShaderSpecialization(const ShaderSpecialization& other);
    ShaderSpecialization(ShaderSpecialization&& other) noexcept;
    ShaderSpecialization& operator=(ShaderSpecialization other);

    std::vector<vk::SpecializationMapEntry> map;
    Bytes data;

    vk::SpecializationInfo info;
};

struct ShaderModule
{
    vk::ShaderStageFlagBits stage;
    vk::ShaderModule module;
    std::optional<ShaderSpecialization> specialization;
};

namespace ShaderHelpers
{
    std::vector<vk::PipelineShaderStageCreateInfo> CreateShaderStagesCreateInfo(
            const std::vector<ShaderModule>& shaderModules);

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
