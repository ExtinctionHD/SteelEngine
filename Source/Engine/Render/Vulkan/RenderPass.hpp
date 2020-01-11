#pragma once

#include "Device.hpp"

class RenderPass
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

    static std::unique_ptr<RenderPass> Create(std::shared_ptr<Device> device,
            const std::vector<Attachment> &attachments, vk::SampleCountFlagBits sampleCount,
            vk::PipelineBindPoint bindPoint);

    RenderPass(std::shared_ptr<Device> aDevice, vk::RenderPass aRenderPass);
    ~RenderPass();

    vk::RenderPass Get() const { return renderPass; }

private:
    std::shared_ptr<Device> device;

    vk::RenderPass renderPass;
};
