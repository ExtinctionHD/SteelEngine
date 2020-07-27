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
    std::vector<vk::PipelineShaderStageCreateInfo> CreateShaderStagesCreateInfo(
            const std::vector<ShaderModule> &shaderModules);
}
