#pragma once
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"

class Renderer
{
public:
    Renderer(const entt::registry& registry);

private:
    struct PipelineState
    {
        std::unique_ptr<GraphicsPipeline> pipeline;
        std::vector<uint32_t> materialIndices;
    };

    vk::Buffer materialBuffer;


};
