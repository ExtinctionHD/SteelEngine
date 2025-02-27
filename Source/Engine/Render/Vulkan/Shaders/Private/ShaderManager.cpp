#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"

#include "Engine/ConsoleVariable.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Filesystem/Filesystem.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    static std::string shadersDirectory = "~/Shaders/";
    static CVarString shadersDirectoryCVar("r.ShadersDirectory", shadersDirectory);

    std::string PreprocessCode(const std::string& code, const ShaderDefines& defines)
    {
        std::istringstream stream(code);

        std::string result;

        std::string line;
        while (std::getline(stream, line))
        {
            if (line.find("#define") == std::string::npos)
            {
                result += line + "\n";
                continue;
            }

            std::string defineLine = line + "\n";

            for (const auto& [name, value] : defines)
            {
                const std::string declaration = "#define " + name + " ";

                if (line.find(declaration) != std::string::npos)
                {
                    defineLine = declaration + std::to_string(value) + "\n";
                }
            }

            result += defineLine;
        }

        return result;
    }
}

ShaderManager::ShaderManager()
    : baseDirectory(Details::shadersDirectory)
{
    Assert(baseDirectory.IsDirectory());

    ShaderCompiler::Initialize();

    Details::shadersDirectoryCVar.SetCallback([&](const CVarString&)
        {
            baseDirectory = Filepath(Details::shadersDirectory);
        });
}

ShaderManager::~ShaderManager()
{
    ShaderCompiler::Finalize();

    Details::shadersDirectoryCVar.SetCallback(nullptr);
}

ShaderModule ShaderManager::CreateShaderModule(const Filepath& filepath,
        vk::ShaderStageFlagBits stage, const ShaderDefines& defines) const
{
    Assert(filepath.Exists() && filepath.Includes(baseDirectory));

    const std::string glslCode = Details::PreprocessCode(Filesystem::ReadFile(filepath), defines);

    const std::vector<uint32_t> spirvCode = ShaderCompiler::Compile(glslCode, stage, baseDirectory.GetAbsolute());

    const vk::ShaderModuleCreateInfo createInfo({}, spirvCode.size() * sizeof(uint32_t), spirvCode.data());
    const auto [result, module] = VulkanContext::device->Get().createShaderModule(createInfo);
    Assert(result == vk::Result::eSuccess);

    return ShaderModule{ module, stage, ShaderSpecialization(), ShaderHelpers::RetrieveShaderReflection(spirvCode) };
}

ShaderModule ShaderManager::CreateComputeShaderModule(const Filepath& filepath,
        const ShaderDefines& defines, const glm::uvec3& workGroupSize) const
{
    ShaderModule shaderModule = CreateShaderModule(filepath, vk::ShaderStageFlagBits::eCompute, defines);

    const std::tuple specializationValues = std::make_tuple(workGroupSize.x, workGroupSize.y, workGroupSize.z);

    shaderModule.specialization = ShaderHelpers::BuildShaderSpecialization(specializationValues);

    return shaderModule;
}

void ShaderManager::DestroyShaderModule(const ShaderModule& shaderModule) const
{
    VulkanContext::device->Get().destroyShaderModule(shaderModule.module);
}
