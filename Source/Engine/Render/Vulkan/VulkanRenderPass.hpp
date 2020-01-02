#pragma once

#include "VulkanDevice.hpp"

class VulkanRenderPass
{
public:
    struct Attachment
    {
        enum class eUsage
        {
            kColor,
            kResolve,
            kDepth
        };

        eUsage usage;
        vk::Format format;
        vk::AttachmentLoadOp loadOp;
        vk::AttachmentStoreOp storeOp;
        vk::ImageLayout initialLayout;
        vk::ImageLayout finalLayout;
    };

    static std::unique_ptr<VulkanRenderPass> Create(std::shared_ptr<VulkanDevice> device,
            const std::vector<Attachment> &attachments, vk::SampleCountFlagBits sampleCount,
            vk::PipelineBindPoint bindPoint);

    VulkanRenderPass(std::shared_ptr<VulkanDevice> aDevice, vk::RenderPass aRenderPass);
    ~VulkanRenderPass();

    vk::RenderPass Get() const { return renderPass; }

private:
    std::shared_ptr<VulkanDevice> device;

    vk::RenderPass renderPass;
};
