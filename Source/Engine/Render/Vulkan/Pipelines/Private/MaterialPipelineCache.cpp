#include "Engine/Render/Vulkan/Pipelines/MaterialPipelineCache.hpp"

#include "Engine/Render/Stages/GBufferStage.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/GraphicsPipeline.hpp"
#include "Engine/Scene/Primitive.hpp"

namespace Details
{
    static const std::map<MaterialPipelineStage, const char*> kShaderNames{
        { MaterialPipelineStage::eGBuffer, "GBuffer" },
        { MaterialPipelineStage::eForward, "Forward " },
    };

    static const std::map<vk::ShaderStageFlagBits, const char*> kShaderExtensions{
        { vk::ShaderStageFlagBits::eVertex, "vert" },
        { vk::ShaderStageFlagBits::eFragment, "frag" },
    };

    static Filepath GetShaderPath(MaterialPipelineStage pipelineStage, vk::ShaderStageFlagBits shaderStage)
    {
        const char* shaderName = kShaderNames.at(pipelineStage);

        const char* shaderExtension = kShaderExtensions.at(shaderStage);

        return Filepath(Format("~/Shaders/Hybrid/%s.%s", shaderName, shaderExtension));
    }

    static std::vector<ShaderModule> CreateShaderModules(MaterialPipelineStage stage, MaterialFlags flags)
    {
        ShaderDefines shaderDefines = MaterialHelpers::GetShaderDefines(flags);

        if (stage == MaterialPipelineStage::eForward)
        {
            if constexpr (Config::kRayTracingEnabled)
            {
                shaderDefines.emplace("RAY_TRACING_ENABLED", 1);
            }
            if constexpr (Config::kGlobalIlluminationEnabled)
            {
                shaderDefines.emplace("LIGHT_VOLUME_ENABLED", 1);
            }
        }

        std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderManager->CreateShaderModule(
                    Details::GetShaderPath(stage, vk::ShaderStageFlagBits::eVertex),
                    vk::ShaderStageFlagBits::eVertex, shaderDefines),
            VulkanContext::shaderManager->CreateShaderModule(
                    Details::GetShaderPath(stage, vk::ShaderStageFlagBits::eFragment),
                    vk::ShaderStageFlagBits::eFragment, shaderDefines),
        };

        return shaderModules;
    }

    static std::vector<BlendMode> GetBlendModes(MaterialPipelineStage stage, MaterialFlags flags)
    {
        std::vector<BlendMode> blendModes;

        if (stage == MaterialPipelineStage::eGBuffer)
        {
            blendModes = Repeat(BlendMode::eDisabled, GBufferStage::kColorAttachmentCount);
        }
        if (stage == MaterialPipelineStage::eForward)
        {
            if (flags | MaterialFlagBits::eAlphaBlend)
            {
                blendModes.push_back(BlendMode::eAlphaBlend);
            }
            else
            {
                blendModes.push_back(BlendMode::eDisabled);
            }
        }

        return blendModes;
    }

    static std::unique_ptr<GraphicsPipeline> CreatePipeline(
            MaterialFlags flags, MaterialPipelineStage stage, vk::RenderPass pass)
    {
        const std::vector<ShaderModule> shaderModules = Details::CreateShaderModules(stage, flags);

        const vk::CullModeFlagBits cullMode = flags & MaterialFlagBits::eDoubleSided
                ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack;

        const std::vector<BlendMode> blendModes = Details::GetBlendModes(stage, flags);

        const GraphicsPipeline::Description description{
            vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill,
            cullMode,
            vk::FrontFace::eCounterClockwise,
            vk::SampleCountFlagBits::e1,
            vk::CompareOp::eLess,
            shaderModules,
            Primitive::kVertexInputs,
            blendModes
        };

        std::unique_ptr<GraphicsPipeline> pipeline = GraphicsPipeline::Create(pass, description);

        for (const auto& shaderModule : shaderModules)
        {
            VulkanContext::shaderManager->DestroyShaderModule(shaderModule);
        }

        return pipeline;
    }
}

MaterialPipelineCache::MaterialPipelineCache(MaterialPipelineStage stage_, vk::RenderPass pass_)
    : stage(stage_)
    , pass(pass_)
{}

const GraphicsPipeline& MaterialPipelineCache::GetPipeline(MaterialFlags flags)
{
    const auto it = pipelines.find(flags);

    if (it != pipelines.end())
    {
        return *it->second;
    }

    pipelines.emplace(flags, Details::CreatePipeline(flags, stage, pass));

    const GraphicsPipeline& pipeline = *pipelines.at(flags);

    if (!descriptorProvider)
    {
        descriptorSetLayouts = pipeline.GetDescriptorSetLayouts();

        descriptorProvider = pipeline.CreateDescriptorProvider();
    }
    else
    {
        Assert(descriptorSetLayouts == pipeline.GetDescriptorSetLayouts());
    }

    return pipeline;
}

DescriptorProvider& MaterialPipelineCache::GetDescriptorProvider() const
{
    Assert(descriptorProvider);

    return *descriptorProvider;
}

void MaterialPipelineCache::ReloadPipelines()
{
    for (auto& [flags, pipeline] : pipelines)
    {
        pipeline = Details::CreatePipeline(flags, stage, pass);

        Assert(descriptorSetLayouts == pipeline->GetDescriptorSetLayouts());
    }
}
