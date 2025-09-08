#pragma once

#include "Engine/Render/Stages/RenderStage.hpp"
#include "Utils/Helpers.hpp"

class ComputePipeline;
class DescriptorProvider;

class AtmosphereStage : public RenderStage
{
public:
    AtmosphereStage(const SceneRenderContext& context_);

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) override;

    void ReloadShaders() override;

private:
    struct Pipelines
    {
        std::unique_ptr<ComputePipeline> transmittance;
        std::unique_ptr<ComputePipeline> multiScattering;
        std::unique_ptr<ComputePipeline> arial;
        std::unique_ptr<ComputePipeline> sky;

        DEFINE_ARRAY_FUNCTIONS(Pipelines, std::unique_ptr<ComputePipeline>)
    };

    Pipelines pipelines;

    void RenderTransmittanceLut(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void RenderMultiTransmittanceLut(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void RenderArialLut(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void RenderSkyLut(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};
