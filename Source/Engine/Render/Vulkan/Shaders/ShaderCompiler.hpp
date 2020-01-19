#pragma once

#include <optional>

#include <SPIRV/GlslangToSpv.h>

namespace ShaderCompiler
{
    void Initialize();
    void Finalize();

    std::vector<uint32_t> CompileSpirv(const std::string &glslCode,
            vk::ShaderStageFlagBits shaderStage, const std::string &folder);
}
