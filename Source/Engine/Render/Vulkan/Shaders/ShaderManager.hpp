#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderHelpers.hpp"
#include "Engine/Filesystem/Filepath.hpp"

class ShaderManager
{
public:
    ShaderManager(const Filepath& baseDirectory_);
    ~ShaderManager();

    ShaderModule CreateShaderModule(vk::ShaderStageFlagBits stage, const Filepath& filepath) const;

    template <class... Types>
    ShaderModule CreateShaderModule(vk::ShaderStageFlagBits stage, const Filepath& filepath,
            const std::tuple<Types...>& specializationValues) const;

    void DestroyShaderModule(const ShaderModule& shaderModule) const;

private:
    Filepath baseDirectory;
};

template <class... Types>
ShaderModule ShaderManager::CreateShaderModule(vk::ShaderStageFlagBits stage,
        const Filepath& filepath, const std::tuple<Types...>& specializationValues) const
{
    constexpr uint32_t valueCount = static_cast<uint32_t>(std::tuple_size<std::tuple<Types...>>::value);

    ShaderSpecialization specialization;

    uint32_t i = 0;
    uint32_t offset = 0;
    const auto functor = [&](const auto& value)
        {
            const uint32_t size = static_cast<uint32_t>(sizeof(value));

            specialization.map.emplace_back(i, offset, size);

            specialization.data.resize(offset + size);
            std::memcpy(specialization.data.data() + offset, &value, size);

            offset += size;
        };

    std::apply(functor, specializationValues);
    specialization.info = vk::SpecializationInfo(valueCount,
            specialization.map.data(), offset, specialization.data.data());

    ShaderModule shaderModule = CreateShaderModule(stage, filepath);
    shaderModule.specialization = specialization;

    return shaderModule;
}
