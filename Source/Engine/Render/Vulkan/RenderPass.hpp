#pragma once

#include "Device.hpp"

struct AttachmentProperties
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

struct RenderPassProperties
{
    vk::PipelineBindPoint bindPoint;
    vk::SampleCountFlagBits sampleCount;
    std::vector<AttachmentProperties> attachments;
};

class RenderPass
{
public:

    static std::unique_ptr<RenderPass> Create(std::shared_ptr<Device> device, const RenderPassProperties &properties);

    RenderPass(std::shared_ptr<Device> aDevice, vk::RenderPass aRenderPass);
    ~RenderPass();

    vk::RenderPass Get() const { return renderPass; }

private:
    std::shared_ptr<Device> device;

    vk::RenderPass renderPass;
};
