#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"

#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Filesystem/Filesystem.hpp"

#include "Utils/Assert.hpp"

ShaderManager::ShaderManager(const Filepath& baseDirectory_)
    : baseDirectory(baseDirectory_)
{
    Assert(baseDirectory.IsDirectory());

    ShaderCompiler::Initialize();
}

ShaderManager::~ShaderManager()
{
    ShaderCompiler::Finalize();
}

ShaderModule ShaderManager::CreateShaderModule(vk::ShaderStageFlagBits stage, const Filepath& filepath) const
{
    Assert(filepath.Exists() && filepath.Includes(baseDirectory));

    const std::string glslCode = Filesystem::ReadFile(filepath);
    const std::vector<uint32_t> spirvCode = ShaderCompiler::Compile(glslCode, stage, baseDirectory.GetAbsolute());

    const vk::ShaderModuleCreateInfo createInfo({}, spirvCode.size() * sizeof(uint32_t), spirvCode.data());
    const auto [result, module] = VulkanContext::device->Get().createShaderModule(createInfo);
    Assert(result == vk::Result::eSuccess);

    const ShaderModule shaderModule{ stage, module, std::nullopt };

    return shaderModule;
}

void ShaderManager::DestroyShaderModule(const ShaderModule& shaderModule) const
{
    VulkanContext::device->Get().destroyShaderModule(shaderModule.module);
}
