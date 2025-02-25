#pragma once

#include "Engine/Render/Stages/RenderStage.hpp"
#include "Utils/Helpers.hpp"

class ComputePipeline;
class DescriptorProvider;

class AtmosphereStage : public RenderStage
{
public:
    AtmosphereStage(const SceneRenderContext& context_);

    ~AtmosphereStage() override;

    void RegisterScene(const Scene* scene_) override;

    void RemoveScene() override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const override;

    void ReloadShaders() override;

private:
    struct DescriptorProviders
    {
        std::unique_ptr<DescriptorProvider> transmittance;
        std::unique_ptr<DescriptorProvider> multiScattering;
        std::unique_ptr<DescriptorProvider> arial;
        std::unique_ptr<DescriptorProvider> sky;

        DEFINE_ARRAY_FUNCTIONS(DescriptorProviders, std::unique_ptr<DescriptorProvider>)

        void Clear() const;
    };

    struct Pipelines
    {
        std::unique_ptr<ComputePipeline> transmittance;
        std::unique_ptr<ComputePipeline> multiScattering;
        std::unique_ptr<ComputePipeline> arial;
        std::unique_ptr<ComputePipeline> sky;

        DEFINE_ARRAY_FUNCTIONS(Pipelines, std::unique_ptr<ComputePipeline>)

        DescriptorProviders CreateDescriptorProviders() const;
    };

    Pipelines pipelines;

    DescriptorProviders descriptorProviders;

    void RenderTransmittanceLut(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void RenderMultiTransmittanceLut(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void RenderArialLut(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void RenderSkyLut(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};
