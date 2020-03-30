#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

struct AttachmentDescription
{
    enum class Usage
    {
        eColor,
        eResolve,
        eDepth
    };

    Usage usage;
    vk::Format format;
    vk::AttachmentLoadOp loadOp;
    vk::AttachmentStoreOp storeOp;
    vk::ImageLayout initialLayout;
    vk::ImageLayout actualLayout;
    vk::ImageLayout finalLayout;
};

struct RenderPassDescription
{
    vk::PipelineBindPoint bindPoint;
    vk::SampleCountFlagBits sampleCount;
    std::vector<AttachmentDescription> attachments;
};

struct RenderPassDependencies
{
    std::optional<PipelineBarrier> previous;
    std::optional<PipelineBarrier> following;
};

class RenderPass
{
public:
    static std::unique_ptr<RenderPass> Create(const RenderPassDescription &description,
            const RenderPassDependencies &dependencies);

    ~RenderPass();

    vk::RenderPass Get() const { return renderPass; }

private:
    vk::RenderPass renderPass;

    RenderPass(vk::RenderPass renderPass_);
};
