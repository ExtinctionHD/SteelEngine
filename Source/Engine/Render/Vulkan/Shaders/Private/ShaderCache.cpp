#include "Engine/Render/Vulkan/Shaders/ShaderCache.hpp"

#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

ShaderCache::ShaderCache(const Filepath &baseDirectory_)
    : baseDirectory(baseDirectory_)
{
    Assert(baseDirectory.IsDirectory());
}

ShaderCache::~ShaderCache()
{
    for (auto &[filepath, shaderModule] : shaderCache)
    {
        VulkanContext::device->Get().destroyShaderModule(shaderModule.module);
    }
}

ShaderModule ShaderCache::CreateShaderModule(vk::ShaderStageFlagBits stage, const Filepath &filepath)
{
    Assert(filepath.Exists() && filepath.Includes(baseDirectory));

    const auto it = shaderCache.find(filepath);
    if (it != shaderCache.end())
    {
        return it->second;
    }

    const std::string glslCode = Filesystem::ReadFile(filepath.GetAbsolute());
    const std::vector<uint32_t> spirvCode = ShaderCompiler::Compile(glslCode, stage, baseDirectory.GetAbsolute());

    const vk::ShaderModuleCreateInfo createInfo({}, spirvCode.size() * sizeof(uint32_t), spirvCode.data());
    const auto [result, module] = VulkanContext::device->Get().createShaderModule(createInfo);
    Assert(result == vk::Result::eSuccess);

    const ShaderModule shaderModule{ stage, module };

    shaderCache.emplace(filepath, shaderModule);

    return shaderModule;
}
