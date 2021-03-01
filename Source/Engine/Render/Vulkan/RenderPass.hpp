#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

class RenderPass
{
public:
    enum class AttachmentUsage
    {
        eColor,
        eResolve,
        eDepth
    };

    struct AttachmentDescription
    {
        AttachmentUsage usage;
        vk::Format format;
        vk::AttachmentLoadOp loadOp;
        vk::AttachmentStoreOp storeOp;
        vk::ImageLayout initialLayout;
        vk::ImageLayout actualLayout;
        vk::ImageLayout finalLayout;
    };

    struct Description
    {
        vk::PipelineBindPoint bindPoint;
        vk::SampleCountFlagBits sampleCount;
        std::vector<AttachmentDescription> attachments;
    };

    struct Dependencies
    {
        std::vector<PipelineBarrier> previous;
        std::vector<PipelineBarrier> following;
    };

    static std::unique_ptr<RenderPass> Create(const Description& description, const Dependencies& dependencies);

    ~RenderPass();

    vk::RenderPass Get() const { return renderPass; }

private:
    vk::RenderPass renderPass;

    RenderPass(vk::RenderPass renderPass_);
};
