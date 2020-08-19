#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"

#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"

ShaderSpecialization::ShaderSpecialization(const ShaderSpecialization& other)
    : map(other.map)
    , data(other.data)
{
    info = other.info;
    info.pMapEntries = map.data();
    info.pData = data.data();
}

ShaderSpecialization::ShaderSpecialization(ShaderSpecialization&& other) noexcept
    : map(std::move(other.map))
    , data(std::move(other.data))
    , info(other.info)
{}

ShaderSpecialization& ShaderSpecialization::operator=(ShaderSpecialization other)
{
    if (this != &other)
    {
        std::swap(map, other.map);
        std::swap(data, other.data);
        std::swap(info, other.info);
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

        if (shaderModule.specialization.has_value())
        {
            pSpecializationInfo = &shaderModule.specialization.value().info;
        }

        createInfo.emplace_back(vk::PipelineShaderStageCreateFlags(),
                shaderModule.stage, shaderModule.module, "main", pSpecializationInfo);
    }

    return createInfo;
}
