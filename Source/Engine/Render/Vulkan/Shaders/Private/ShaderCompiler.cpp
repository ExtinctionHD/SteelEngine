PRAGMA_DISABLE_WARNINGS
#include <ResourceLimits.h>
#include <ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>
PRAGMA_ENABLE_WARNINGS

#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    static constexpr int32_t kInputVersion = 100;
    static constexpr int32_t kDefaultVersion = 100;
    static constexpr EShMessages kDefaultMessages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    static constexpr TBuiltInResource kDefaultResource = {
        .maxLights = 32,
        .maxClipPlanes = 6,
        .maxTextureUnits = 32,
        .maxTextureCoords = 32,
        .maxVertexAttribs = 64,
        .maxVertexUniformComponents = 4096,
        .maxVaryingFloats = 64,
        .maxVertexTextureImageUnits = 32,
        .maxCombinedTextureImageUnits = 80,
        .maxTextureImageUnits = 32,
        .maxFragmentUniformComponents = 4096,
        .maxDrawBuffers = 32,
        .maxVertexUniformVectors = 128,
        .maxVaryingVectors = 8,
        .maxFragmentUniformVectors = 16,
        .maxVertexOutputVectors = 16,
        .maxFragmentInputVectors = 15,
        .minProgramTexelOffset = -8,
        .maxProgramTexelOffset = 7,
        .maxClipDistances = 8,
        .maxComputeWorkGroupCountX = 65535,
        .maxComputeWorkGroupCountY = 65535,
        .maxComputeWorkGroupCountZ = 65535,
        .maxComputeWorkGroupSizeX = 1024,
        .maxComputeWorkGroupSizeY = 1024,
        .maxComputeWorkGroupSizeZ = 64,
        .maxComputeUniformComponents = 1024,
        .maxComputeTextureImageUnits = 16,
        .maxComputeImageUniforms = 8,
        .maxComputeAtomicCounters = 8,
        .maxComputeAtomicCounterBuffers = 1,
        .maxVaryingComponents = 60,
        .maxVertexOutputComponents = 64,
        .maxGeometryInputComponents = 64,
        .maxGeometryOutputComponents = 128,
        .maxFragmentInputComponents = 128,
        .maxImageUnits = 8,
        .maxCombinedImageUnitsAndFragmentOutputs = 8,
        .maxCombinedShaderOutputResources = 8,
        .maxImageSamples = 0,
        .maxVertexImageUniforms = 0,
        .maxTessControlImageUniforms = 0,
        .maxTessEvaluationImageUniforms = 0,
        .maxGeometryImageUniforms = 0,
        .maxFragmentImageUniforms = 8,
        .maxCombinedImageUniforms = 8,
        .maxGeometryTextureImageUnits = 16,
        .maxGeometryOutputVertices = 256,
        .maxGeometryTotalOutputComponents = 1024,
        .maxGeometryUniformComponents = 1024,
        .maxGeometryVaryingComponents = 64,
        .maxTessControlInputComponents = 128,
        .maxTessControlOutputComponents = 128,
        .maxTessControlTextureImageUnits = 16,
        .maxTessControlUniformComponents = 1024,
        .maxTessControlTotalOutputComponents = 4096,
        .maxTessEvaluationInputComponents = 128,
        .maxTessEvaluationOutputComponents = 128,
        .maxTessEvaluationTextureImageUnits = 16,
        .maxTessEvaluationUniformComponents = 1024,
        .maxTessPatchComponents = 120,
        .maxPatchVertices = 32,
        .maxTessGenLevel = 64,
        .maxViewports = 16,
        .maxVertexAtomicCounters = 0,
        .maxTessControlAtomicCounters = 0,
        .maxTessEvaluationAtomicCounters = 0,
        .maxGeometryAtomicCounters = 0,
        .maxFragmentAtomicCounters = 8,
        .maxCombinedAtomicCounters = 8,
        .maxAtomicCounterBindings = 1,
        .maxVertexAtomicCounterBuffers = 0,
        .maxTessControlAtomicCounterBuffers = 0,
        .maxTessEvaluationAtomicCounterBuffers = 0,
        .maxGeometryAtomicCounterBuffers = 0,
        .maxFragmentAtomicCounterBuffers = 1,
        .maxCombinedAtomicCounterBuffers = 1,
        .maxAtomicCounterBufferSize = 16384,
        .maxTransformFeedbackBuffers = 4,
        .maxTransformFeedbackInterleavedComponents = 64,
        .maxCullDistances = 8,
        .maxCombinedClipAndCullDistances = 8,
        .maxSamples = 4,
        .maxMeshOutputVerticesNV = 256,
        .maxMeshOutputPrimitivesNV = 512,
        .maxMeshWorkGroupSizeX_NV = 32,
        .maxMeshWorkGroupSizeY_NV = 1,
        .maxMeshWorkGroupSizeZ_NV = 1,
        .maxTaskWorkGroupSizeX_NV = 32,
        .maxTaskWorkGroupSizeY_NV = 1,
        .maxTaskWorkGroupSizeZ_NV = 1,
        .maxMeshViewCountNV = 4,
        .limits = {
            .nonInductiveForLoops = true,
            .whileLoops = true,
            .doWhileLoops = true,
            .generalUniformIndexing = true,
            .generalAttributeMatrixVectorIndexing = true,
            .generalVaryingIndexing = true,
            .generalSamplerIndexing = true,
            .generalVariableIndexing = true,
            .generalConstantMatrixVectorIndexing = true,
        }
    };

    static constexpr glslang::EShTargetClientVersion kClientVersion = glslang::EShTargetVulkan_1_2;
    static constexpr glslang::EShTargetLanguageVersion kTargetVersion = glslang::EShTargetSpv_1_5;


    static bool initialized = false;

    static EShLanguage TranslateShaderStage(vk::ShaderStageFlagBits shaderStage)
    {
        switch (shaderStage)
        {
        case vk::ShaderStageFlagBits::eVertex: return EShLangVertex;
        case vk::ShaderStageFlagBits::eTessellationControl: return EShLangTessControl;
        case vk::ShaderStageFlagBits::eTessellationEvaluation: return EShLangTessEvaluation;
        case vk::ShaderStageFlagBits::eGeometry: return EShLangGeometry;
        case vk::ShaderStageFlagBits::eFragment: return EShLangFragment;
        case vk::ShaderStageFlagBits::eCompute: return EShLangCompute;
        case vk::ShaderStageFlagBits::eRaygenKHR: return EShLangRayGen;
        case vk::ShaderStageFlagBits::eAnyHitKHR: return EShLangAnyHit;
        case vk::ShaderStageFlagBits::eClosestHitKHR: return EShLangClosestHit;
        case vk::ShaderStageFlagBits::eMissKHR: return EShLangMiss;
        case vk::ShaderStageFlagBits::eIntersectionKHR: return EShLangIntersect;
        case vk::ShaderStageFlagBits::eCallableKHR: return EShLangCallable;
        case vk::ShaderStageFlagBits::eTaskNV: return EShLangTaskNV;
        case vk::ShaderStageFlagBits::eMeshNV: return EShLangMeshNV;
        default:
            Assert(false);
            return {};
        }
    }
}

void ShaderCompiler::Initialize()
{
    if (!Details::initialized)
    {
        glslang::InitializeProcess();
        Details::initialized = true;
    }
}

void ShaderCompiler::Finalize()
{
    if (Details::initialized)
    {
        glslang::FinalizeProcess();
        Details::initialized = false;
    }
}

std::vector<uint32_t> ShaderCompiler::Compile(const std::string& glslCode,
        vk::ShaderStageFlagBits shaderStage, const std::string& folder)
{
    Assert(Details::initialized);

    const EShLanguage stage = Details::TranslateShaderStage(shaderStage);
    const char* shaderString = glslCode.data();

    glslang::TShader shader(stage);
    shader.setStrings(&shaderString, 1);

    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, Details::kInputVersion);
    shader.setEnvClient(glslang::EShClientVulkan, Details::kClientVersion);
    shader.setEnvTarget(glslang::EShTargetSpv, Details::kTargetVersion);

    DirStackFileIncluder includer;
    includer.pushExternalLocalDirectory(folder);

    const char* glslCodeCStr = glslCode.c_str();
    shader.setStrings(&glslCodeCStr, 1);

    if (!shader.parse(&Details::kDefaultResource, Details::kDefaultVersion,
            false, Details::kDefaultMessages, includer))
    {
        LogE << "Failed to parse shader:\n" << shader.getInfoLog() << shader.getInfoDebugLog() << "\n";
        Assert(false);
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(Details::kDefaultMessages))
    {
        LogE << "Failed to link shader:\n" << shader.getInfoLog() << shader.getInfoDebugLog() << "\n";
        Assert(false);
    }

    std::vector<uint32_t> spirv;
    spv::SpvBuildLogger logger;
    GlslangToSpv(*program.getIntermediate(stage), spirv, &logger);

    const std::string messages = logger.getAllMessages();
    if (!messages.empty())
    {
        LogI << "Spirv compiler messages:\n" << messages;
    }

    return spirv;
}
