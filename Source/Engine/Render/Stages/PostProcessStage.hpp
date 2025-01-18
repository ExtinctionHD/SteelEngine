#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"

class Scene;
class ComputePipeline;

class PostProcessStage
{
public:
    PostProcessStage();

    ~PostProcessStage();

    void RegisterScene(const Scene* scene_);

    void RemoveScene();

    void Update() const;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize() const;

    void ReloadShaders();

private:
    const Scene* scene = nullptr;

    std::unique_ptr<ComputePipeline> pipeline;
    std::unique_ptr<DescriptorProvider> descriptorProvider;
};
