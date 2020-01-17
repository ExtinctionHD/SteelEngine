#pragma once

#include <optional>

#include <SPIRV/GlslangToSpv.h>

namespace ShaderCompiler
{
    void Initialize();
    void Finalize();

    std::optional<std::vector<uint32_t>> Compile(const std::string &glslCode, vk::ShaderStageFlagBits shaderStage);
}
