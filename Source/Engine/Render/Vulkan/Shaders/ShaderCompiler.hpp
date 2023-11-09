#pragma once

namespace ShaderCompiler
{
    void Initialize();
    void Finalize();

    std::vector<uint32_t> Compile(
            const std::string& glslCode, vk::ShaderStageFlagBits shaderStage, const std::string& folder);
}
