#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
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
    std::optional<MemoryDependency> previous;
    std::optional<MemoryDependency> following;
};

class RenderPass
{
public:
    static std::unique_ptr<RenderPass> Create(std::shared_ptr<Device> device,
            const RenderPassDescription &description, const RenderPassDependencies &dependencies);

    ~RenderPass();

    vk::RenderPass Get() const { return renderPass; }

private:
    std::shared_ptr<Device> device;

    vk::RenderPass renderPass;

    RenderPass(std::shared_ptr<Device> device_, vk::RenderPass renderPass_);
};
