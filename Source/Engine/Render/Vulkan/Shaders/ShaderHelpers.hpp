#pragma once

#include "Utils/DataHelpers.hpp"

struct ShaderSpecialization
{
    ShaderSpecialization() = default;
    ~ShaderSpecialization() = default;
    ShaderSpecialization(const ShaderSpecialization &other);
    ShaderSpecialization(ShaderSpecialization &&other) noexcept;
    ShaderSpecialization &operator=(ShaderSpecialization other);

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
    template <class... Types>
    ShaderSpecialization CreateShaderSpecialization(const std::tuple<Types...> &values);

    std::vector<vk::PipelineShaderStageCreateInfo> CreateShaderStagesCreateInfo(
            const std::vector<ShaderModule> &shaderModules);
}

template <class... Types>
ShaderSpecialization ShaderHelpers::CreateShaderSpecialization(const std::tuple<Types...> &values)
{
    constexpr uint32_t count = static_cast<uint32_t>(std::tuple_size<std::tuple<Types...>>::value);

    ShaderSpecialization specialization;

    uint32_t i = 0;
    uint32_t offset = 0;
    const auto functor = [&](const auto &value)
        {
            const uint32_t size = static_cast<uint32_t>(sizeof(value));

            specialization.map.emplace_back(i, offset, size);

            specialization.data.resize(offset + size);
            std::memcpy(specialization.data.data() + offset, &value, size);

            offset += size;
        };

    std::apply(functor, values);

    specialization.info = vk::SpecializationInfo(count,
            specialization.map.data(), offset, specialization.data.data());

    return specialization;
}
