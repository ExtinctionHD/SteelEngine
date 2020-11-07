#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/System/System.hpp"

class SceneRT;
class Camera;
class RenderObject;
struct KeyInput;

namespace RT
{
    class RayTracingPipeline;

    class RenderSystem
        : public System
    {
    public:
        RenderSystem(SceneRT* scene_);
        ~RenderSystem();

        void Process(float deltaSeconds) override;

        void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

    private:
        struct RenderTargets
        {
            MultiDescriptorSet descriptorSet;
        };

        struct AccumulationTarget
        {
            vk::Image image;
            vk::ImageView view;
            DescriptorSet descriptorSet;
            uint32_t accumulationCount = 0;
        };

        SceneRT* scene = nullptr;

        RenderTargets renderTargets;
        AccumulationTarget accumulationTarget;

        std::unique_ptr<RT::RayTracingPipeline> rayTracingPipeline;
        std::vector<vk::DescriptorSet> descriptorSets;

        void SetupRenderTargets();
        void SetupAccumulationTarget();
        void SetupRayTracingPipeline();
        void SetupDescriptorSets();

        void HandleResizeEvent(const vk::Extent2D& extent);

        void HandleKeyInputEvent(const KeyInput& keyInput);

        void ReloadShaders();

        void ResetAccumulation();
    };
}
