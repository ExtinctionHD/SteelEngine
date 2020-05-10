#include "Engine/Render/Vulkan/RenderPass.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

namespace SRenderPass
{
    vk::SubpassDependency GetSubpassDependency(uint32_t srcSubpass, uint32_t dstSubpass,
            const PipelineBarrier &pipelineBarrier)
    {
        return vk::SubpassDependency(srcSubpass, dstSubpass,
                pipelineBarrier.waitedScope.stages, pipelineBarrier.blockedScope.stages,
                pipelineBarrier.waitedScope.access, pipelineBarrier.blockedScope.access,
                vk::DependencyFlags());
    }

    std::vector<vk::SubpassDependency> GetSubpassDependencies(const RenderPass::Dependencies &dependencies)
    {
        std::vector<vk::SubpassDependency> subpassDependencies;
        subpassDependencies.reserve(2);

        if (dependencies.previous.has_value())
        {
            subpassDependencies.push_back(GetSubpassDependency(VK_SUBPASS_EXTERNAL, 0, dependencies.previous.value()));
        }

        if (dependencies.following.has_value())
        {
            subpassDependencies.push_back(GetSubpassDependency(0, VK_SUBPASS_EXTERNAL, dependencies.following.value()));
        }

        return subpassDependencies;
    }
}

std::unique_ptr<RenderPass> RenderPass::Create(const Description &description,
        const Dependencies &dependencies)
{
    const std::vector<Attachment> &attachments = description.attachments;

    std::vector<vk::AttachmentDescription> attachmentDescriptions;
    attachmentDescriptions.reserve(attachments.size());

    uint32_t colorAttachmentCount = 0;
    uint32_t resolveAttachmentCount = 0;
    uint32_t depthAttachmentCount = 0;

    for (const auto &attachment : attachments)
    {
        switch (attachment.usage)
        {
        case Attachment::Usage::eColor:
            ++colorAttachmentCount;
            break;
        case Attachment::Usage::eResolve:
            ++resolveAttachmentCount;
            break;
        case Attachment::Usage::eDepth:
            ++depthAttachmentCount;
            break;
        default:
            Assert(false);
            break;
        }

        attachmentDescriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                attachment.format, description.sampleCount,
                attachment.loadOp, attachment.storeOp,
                attachment.loadOp, attachment.storeOp,
                attachment.initialLayout, attachment.finalLayout);
    }

    Assert(resolveAttachmentCount == 0 || colorAttachmentCount == resolveAttachmentCount);
    Assert(depthAttachmentCount <= 1);

    std::vector<vk::AttachmentReference> colorAttachmentReferences;
    colorAttachmentReferences.reserve(colorAttachmentCount);
    std::vector<vk::AttachmentReference> resolveAttachmentReferences;
    colorAttachmentReferences.reserve(resolveAttachmentCount);
    std::unique_ptr<vk::AttachmentReference> depthAttachmentReference;

    for (uint32_t i = 0; i < attachments.size(); ++i)
    {
        const vk::AttachmentReference attachmentReference(i, attachments[i].actualLayout);

        switch (attachments[i].usage)
        {
        case Attachment::Usage::eColor:
            colorAttachmentReferences.push_back(attachmentReference);
            break;
        case Attachment::Usage::eResolve:
            resolveAttachmentReferences.push_back(attachmentReference);
            break;
        case Attachment::Usage::eDepth:
            depthAttachmentReference = std::make_unique<vk::AttachmentReference>(attachmentReference);
            break;
        default:
            Assert(false);
            break;
        }
    }

    const vk::SubpassDescription subpassDescription(
            vk::SubpassDescriptionFlags(), description.bindPoint,
            0, nullptr,
            colorAttachmentCount, colorAttachmentReferences.data(),
            resolveAttachmentReferences.empty() ? nullptr : resolveAttachmentReferences.data(),
            depthAttachmentReference.get(),
            0, nullptr);

    const std::vector<vk::SubpassDependency> subpassDependencies = SRenderPass::GetSubpassDependencies(dependencies);

    const vk::RenderPassCreateInfo createInfo({},
            static_cast<uint32_t>(attachmentDescriptions.size()), attachmentDescriptions.data(),
            1, &subpassDescription, static_cast<uint32_t>(subpassDependencies.size()), subpassDependencies.data());

    const auto [result, renderPass] = VulkanContext::device->Get().createRenderPass(createInfo);
    Assert(result == vk::Result::eSuccess);

    return std::unique_ptr<RenderPass>(new RenderPass(renderPass));
}

RenderPass::RenderPass(vk::RenderPass renderPass_)
    : renderPass(renderPass_)
{}

RenderPass::~RenderPass()
{
    VulkanContext::device->Get().destroyRenderPass(renderPass);
}
