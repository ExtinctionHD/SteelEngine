#include <StandAlone/DirStackFileIncluder.h>

#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"

#include "Utils/Assert.hpp"

namespace SShaderCompiler
{
    constexpr int kInputVersion = 100;
    constexpr glslang::EShTargetClientVersion kClientVersion = glslang::EShTargetVulkan_1_0;
    constexpr glslang::EShTargetLanguageVersion kTargetVersion = glslang::EShTargetSpv_1_0;

    constexpr int kDefaultVersion = 100;
    constexpr TBuiltInResource kDefaultResource = {
        /* .MaxLights = */ 32,
        /* .MaxClipPlanes = */ 6,
        /* .MaxTextureUnits = */ 32,
        /* .MaxTextureCoords = */ 32,
        /* .MaxVertexAttribs = */ 64,
        /* .MaxVertexUniformComponents = */ 4096,
        /* .MaxVaryingFloats = */ 64,
        /* .MaxVertexTextureImageUnits = */ 32,
        /* .MaxCombinedTextureImageUnits = */ 80,
        /* .MaxTextureImageUnits = */ 32,
        /* .MaxFragmentUniformComponents = */ 4096,
        /* .MaxDrawBuffers = */ 32,
        /* .MaxVertexUniformVectors = */ 128,
        /* .MaxVaryingVectors = */ 8,
        /* .MaxFragmentUniformVectors = */ 16,
        /* .MaxVertexOutputVectors = */ 16,
        /* .MaxFragmentInputVectors = */ 15,
        /* .MinProgramTexelOffset = */ -8,
        /* .MaxProgramTexelOffset = */ 7,
        /* .MaxClipDistances = */ 8,
        /* .MaxComputeWorkGroupCountX = */ 65535,
        /* .MaxComputeWorkGroupCountY = */ 65535,
        /* .MaxComputeWorkGroupCountZ = */ 65535,
        /* .MaxComputeWorkGroupSizeX = */ 1024,
        /* .MaxComputeWorkGroupSizeY = */ 1024,
        /* .MaxComputeWorkGroupSizeZ = */ 64,
        /* .MaxComputeUniformComponents = */ 1024,
        /* .MaxComputeTextureImageUnits = */ 16,
        /* .MaxComputeImageUniforms = */ 8,
        /* .MaxComputeAtomicCounters = */ 8,
        /* .MaxComputeAtomicCounterBuffers = */ 1,
        /* .MaxVaryingComponents = */ 60,
        /* .MaxVertexOutputComponents = */ 64,
        /* .MaxGeometryInputComponents = */ 64,
        /* .MaxGeometryOutputComponents = */ 128,
        /* .MaxFragmentInputComponents = */ 128,
        /* .MaxImageUnits = */ 8,
        /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
        /* .MaxCombinedShaderOutputResources = */ 8,
        /* .MaxImageSamples = */ 0,
        /* .MaxVertexImageUniforms = */ 0,
        /* .MaxTessControlImageUniforms = */ 0,
        /* .MaxTessEvaluationImageUniforms = */ 0,
        /* .MaxGeometryImageUniforms = */ 0,
        /* .MaxFragmentImageUniforms = */ 8,
        /* .MaxCombinedImageUniforms = */ 8,
        /* .MaxGeometryTextureImageUnits = */ 16,
        /* .MaxGeometryOutputVertices = */ 256,
        /* .MaxGeometryTotalOutputComponents = */ 1024,
        /* .MaxGeometryUniformComponents = */ 1024,
        /* .MaxGeometryVaryingComponents = */ 64,
        /* .MaxTessControlInputComponents = */ 128,
        /* .MaxTessControlOutputComponents = */ 128,
        /* .MaxTessControlTextureImageUnits = */ 16,
        /* .MaxTessControlUniformComponents = */ 1024,
        /* .MaxTessControlTotalOutputComponents = */ 4096,
        /* .MaxTessEvaluationInputComponents = */ 128,
        /* .MaxTessEvaluationOutputComponents = */ 128,
        /* .MaxTessEvaluationTextureImageUnits = */ 16,
        /* .MaxTessEvaluationUniformComponents = */ 1024,
        /* .MaxTessPatchComponents = */ 120,
        /* .MaxPatchVertices = */ 32,
        /* .MaxTessGenLevel = */ 64,
        /* .MaxViewports = */ 16,
        /* .MaxVertexAtomicCounters = */ 0,
        /* .MaxTessControlAtomicCounters = */ 0,
        /* .MaxTessEvaluationAtomicCounters = */ 0,
        /* .MaxGeometryAtomicCounters = */ 0,
        /* .MaxFragmentAtomicCounters = */ 8,
        /* .MaxCombinedAtomicCounters = */ 8,
        /* .MaxAtomicCounterBindings = */ 1,
        /* .MaxVertexAtomicCounterBuffers = */ 0,
        /* .MaxTessControlAtomicCounterBuffers = */ 0,
        /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
        /* .MaxGeometryAtomicCounterBuffers = */ 0,
        /* .MaxFragmentAtomicCounterBuffers = */ 1,
        /* .MaxCombinedAtomicCounterBuffers = */ 1,
        /* .MaxAtomicCounterBufferSize = */ 16384,
        /* .MaxTransformFeedbackBuffers = */ 4,
        /* .MaxTransformFeedbackInterleavedComponents = */ 64,
        /* .MaxCullDistances = */ 8,
        /* .MaxCombinedClipAndCullDistances = */ 8,
        /* .MaxSamples = */ 4,
        /* .maxMeshOutputVerticesNV = */ 256,
        /* .maxMeshOutputPrimitivesNV = */ 512,
        /* .maxMeshWorkGroupSizeX_NV = */ 32,
        /* .maxMeshWorkGroupSizeY_NV = */ 1,
        /* .maxMeshWorkGroupSizeZ_NV = */ 1,
        /* .maxTaskWorkGroupSizeX_NV = */ 32,
        /* .maxTaskWorkGroupSizeY_NV = */ 1,
        /* .maxTaskWorkGroupSizeZ_NV = */ 1,
        /* .maxMeshViewCountNV = */ 4,

        /* .limits = */ {
            /* .nonInductiveForLoops = */ true,
            /* .whileLoops = */ true,
            /* .doWhileLoops = */ true,
            /* .generalUniformIndexing = */ true,
            /* .generalAttributeMatrixVectorIndexing = */ true,
            /* .generalVaryingIndexing = */ true,
            /* .generalSamplerIndexing = */ true,
            /* .generalVariableIndexing = */ true,
            /* .generalConstantMatrixVectorIndexing = */ true,
        }
    };
    constexpr EShMessages kDefaultMessages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    bool initialized = false;

    EShLanguage TranslateShaderStage(vk::ShaderStageFlagBits shaderStage)
    {
        switch (shaderStage)
        {
        case vk::ShaderStageFlagBits::eVertex: return EShLangVertex;
        case vk::ShaderStageFlagBits::eTessellationControl: return EShLangTessControl;
        case vk::ShaderStageFlagBits::eTessellationEvaluation: return EShLangTessEvaluation;
        case vk::ShaderStageFlagBits::eGeometry: return EShLangGeometry;
        case vk::ShaderStageFlagBits::eFragment: return EShLangFragment;
        case vk::ShaderStageFlagBits::eCompute: return EShLangCompute;
        case vk::ShaderStageFlagBits::eRaygenNV: return EShLangRayGenNV;
        case vk::ShaderStageFlagBits::eAnyHitNV: return EShLangAnyHitNV;
        case vk::ShaderStageFlagBits::eClosestHitNV: return EShLangClosestHitNV;
        case vk::ShaderStageFlagBits::eMissNV: return EShLangMissNV;
        case vk::ShaderStageFlagBits::eIntersectionNV: return EShLangIntersectNV;
        case vk::ShaderStageFlagBits::eCallableNV: return EShLangCallableNV;
        case vk::ShaderStageFlagBits::eTaskNV: return EShLangTaskNV;
        case vk::ShaderStageFlagBits::eMeshNV: return EShLangMeshNV;
        default:
            Assert(false);
            return EShLangVertex;
        }
    }
}

void ShaderCompiler::Initialize()
{
    if (!SShaderCompiler::initialized)
    {
        glslang::InitializeProcess();
        SShaderCompiler::initialized = true;
    }
}

void ShaderCompiler::Finalize()
{
    if (SShaderCompiler::initialized)
    {
        glslang::FinalizeProcess();
        SShaderCompiler::initialized = false;
    }
}

std::vector<uint32_t> ShaderCompiler::Compile(const std::string &glslCode,
        vk::ShaderStageFlagBits shaderStage, const std::string &folder)
{
    Assert(SShaderCompiler::initialized);

    const EShLanguage stage = SShaderCompiler::TranslateShaderStage(shaderStage);
    const char *shaderString = glslCode.data();

    glslang::TShader shader(stage);
    shader.setStrings(&shaderString, 1);

    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, SShaderCompiler::kInputVersion);
    shader.setEnvClient(glslang::EShClientVulkan, SShaderCompiler::kClientVersion);
    shader.setEnvTarget(glslang::EShTargetSpv, SShaderCompiler::kTargetVersion);

    DirStackFileIncluder includer;
    includer.pushExternalLocalDirectory(folder);

    std::string preprocessedCode;
    if (!shader.preprocess(&SShaderCompiler::kDefaultResource, SShaderCompiler::kDefaultVersion,
            ENoProfile, false, false, SShaderCompiler::kDefaultMessages, &preprocessedCode, includer))
    {
        LogE << "Failed to preprocess shader:\n" << shader.getInfoLog() << shader.getInfoDebugLog() << "\n";
        Assert(false);
    }

    const char *preprocessedCodeCStr = preprocessedCode.c_str();
    shader.setStrings(&preprocessedCodeCStr, 1);

    if (!shader.parse(&SShaderCompiler::kDefaultResource, SShaderCompiler::kDefaultVersion,
            false, SShaderCompiler::kDefaultMessages))
    {
        LogE << "Failed to parse shader:\n" << shader.getInfoLog() << shader.getInfoDebugLog() << "\n";
        Assert(false);
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(SShaderCompiler::kDefaultMessages))
    {
        LogE << "Failed to link shader:\n" << shader.getInfoLog() << shader.getInfoDebugLog() << "\n";
        Assert(false);
    }

    std::vector<uint32_t> spirv;
    spv::SpvBuildLogger logger;
    GlslangToSpv(*program.getIntermediate(stage), spirv, &logger);

    std::string messages = logger.getAllMessages();
    if (!messages.empty())
    {
        LogI << "Spirv compiler messages:\n" << messages;
    }

    return spirv;
}
