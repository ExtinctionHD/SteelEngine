#include "Engine/Render/Vulkan/Shaders/ShaderCache.hpp"

#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"

#include "Utils/Assert.hpp"

namespace SShaderCache
{
    std::string InsertDefines(const std::string &source, const std::vector<std::pair<std::string, uint32_t>> &defines)
    {
        std::string result;
        for (const auto &[define, value] : defines)
        {
            result += "#define " + define + " " + std::to_string(value) + "\n";
        }
        result += "\n" + source;

        return result;
    }
}

ShaderCache::ShaderCache(std::shared_ptr<Device> aDevice, const Filepath &aBaseDirectory)
    : device(aDevice)
    , baseDirectory(aBaseDirectory)
{
    Assert(baseDirectory.IsDirectory());
}

ShaderCache::~ShaderCache()
{
    for (auto &entry : shaderCache)
    {
        device->Get().destroyShaderModule(entry.shaderModule.module);
    }
}

ShaderModule ShaderCache::CreateShaderModule(vk::ShaderStageFlagBits stage, const Filepath &filepath,
        const std::vector<std::pair<std::string, uint32_t>> &defines)
{
    Assert(filepath.Exists() && filepath.Includes(baseDirectory));

    const auto pred = [&stage, &filepath, &defines](const ShaderCacheEntry &entry)
        {
            return entry.shaderModule.stage == stage && entry.filepath == filepath && entry.defines == defines;
        };

    const auto it = std::find_if(shaderCache.begin(), shaderCache.end(), pred);
    if (it != shaderCache.end())
    {
        return it->shaderModule;
    }

    std::string &glslCode = codeCache[filepath];
    if (glslCode.empty())
    {
        glslCode = Filesystem::ReadFile(filepath.GetAbsolute());
    }

    const std::string glslCodeWithDefines = SShaderCache::InsertDefines(glslCode, defines);
    const std::vector<uint32_t> spirvCode = ShaderCompiler::Compile(
            glslCodeWithDefines, stage, baseDirectory.GetAbsolute());

    const vk::ShaderModuleCreateInfo createInfo({}, spirvCode.size() * sizeof(uint32_t), spirvCode.data());
    const auto [result, module] = device->Get().createShaderModule(createInfo);
    Assert(result == vk::Result::eSuccess);

    shaderCache.push_back({ filepath, defines, { stage, module } });

    return shaderCache.back().shaderModule;
}
