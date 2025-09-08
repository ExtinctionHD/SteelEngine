#pragma once

#include "Engine/Scene/Material.hpp"

class DescriptorProvider;
class GraphicsPipeline;

enum class MaterialPipelineStage
{
    eGBuffer,
    eForward,
};

class MaterialPipelineCache
{
public:
    MaterialPipelineCache(MaterialPipelineStage stage_, vk::RenderPass pass_);

    const GraphicsPipeline& GetPipeline(MaterialFlags flags);

    DescriptorProvider& GetDescriptors() const;

    void ReloadPipelines();

private:
    MaterialPipelineStage stage;
    vk::RenderPass pass;

    std::map<MaterialFlags, std::unique_ptr<GraphicsPipeline>> pipelines;
    std::unique_ptr<DescriptorProvider> descriptors;
};
