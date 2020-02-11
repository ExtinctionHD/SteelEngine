#include "Engine/Render/Vulkan/RenderPass.hpp"

#include "Utils/Assert.hpp"

namespace SRenderPass
{
    vk::SubpassDependency GetSubpassDependency(uint32_t srcSubpass, uint32_t dstSubpass,
            const VulkanHelpers::Dependency &dependency)
    {
        return vk::SubpassDependency(srcSubpass, dstSubpass,
                dependency.srcStageMask, dependency.dstStageMask,
                dependency.srcAccessMask, dependency.dstAccessMask, {});
    }

    std::vector<vk::SubpassDependency> GetSubpassDependencies(const RenderPassDependencies &dependencies)
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

std::unique_ptr<RenderPass> RenderPass::Create(std::shared_ptr<Device> device,
        const RenderPassDescription &description, const RenderPassDependencies &dependencies)
{
    const std::vector<AttachmentDescription> &attachments = description.attachments;

    std::vector<vk::AttachmentDescription> attachmentDescriptions;
    attachmentDescriptions.reserve(attachments.size());

    uint32_t colorAttachmentCount = 0;
    uint32_t resolveAttachmentCount = 0;
    uint32_t depthAttachmentCount = 0;

    for (const auto &attachment : attachments)
    {
        switch (attachment.usage)
        {
        case AttachmentDescription::eUsage::kColor:
            ++colorAttachmentCount;
            break;
        case AttachmentDescription::eUsage::kResolve:
            ++resolveAttachmentCount;
            break;
        case AttachmentDescription::eUsage::kDepth:
            ++depthAttachmentCount;
            break;
        default:
            Assert(false);
            break;
        }

        attachmentDescriptions.push_back(vk::AttachmentDescription({}, attachment.format, description.sampleCount,
                attachment.loadOp, attachment.storeOp, attachment.loadOp, attachment.storeOp,
                attachment.initialLayout, attachment.finalLayout));
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
        const vk::AttachmentReference attachmentReference(i, attachments[i].initialLayout);

        switch (attachments[i].usage)
        {
        case AttachmentDescription::eUsage::kColor:
            colorAttachmentReferences.push_back(attachmentReference);
            break;
        case AttachmentDescription::eUsage::kResolve:
            resolveAttachmentReferences.push_back(attachmentReference);
            break;
        case AttachmentDescription::eUsage::kDepth:
            depthAttachmentReference = std::make_unique<vk::AttachmentReference>(attachmentReference);
            break;
        default:
            Assert(false);
            break;
        }
    }

    const vk::SubpassDescription subpassDescription({}, description.bindPoint,
            0, nullptr,
            colorAttachmentCount, colorAttachmentReferences.data(),
            resolveAttachmentReferences.empty() ? nullptr : resolveAttachmentReferences.data(),
            depthAttachmentReference.get(),
            0, nullptr);

    const std::vector<vk::SubpassDependency> subpassDependencies = SRenderPass::GetSubpassDependencies(dependencies);

    const vk::RenderPassCreateInfo createInfo({},
            static_cast<uint32_t>(attachmentDescriptions.size()), attachmentDescriptions.data(),
            1, &subpassDescription, static_cast<uint32_t>(subpassDependencies.size()), subpassDependencies.data());

    const auto [result, renderPass] = device->Get().createRenderPass(createInfo);
    Assert(result == vk::Result::eSuccess);

    LogD << "RenderPass created" << "\n";

    return std::unique_ptr<RenderPass>(new RenderPass(device, renderPass));
}

RenderPass::RenderPass(std::shared_ptr<Device> aDevice, vk::RenderPass aRenderPass)
    : device(aDevice)
    , renderPass(aRenderPass)
{}

RenderPass::~RenderPass()
{
    device->Get().destroyRenderPass(renderPass);
}
