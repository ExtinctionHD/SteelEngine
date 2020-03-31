#include "Engine/Render/Rasterizer.hpp"

#include "Engine/Camera.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/Shaders/ShaderCompiler.hpp"
#include "Engine/EngineHelpers.hpp"

namespace SRasterizer
{
    const vk::Format kDepthFormat = vk::Format::eD32Sfloat;
    const vk::ClearColorValue kClearColorValue = std::array<float, 4>{ 0.7f, 0.8f, 0.9f, 1.0f };
    const glm::vec4 kLightDirection(Direction::kDown + Direction::kRight + Direction::kForward, 0.0f);

    std::pair<vk::Image, vk::ImageView> CreateDepthAttachment()
    {
        const ImageDescription description{
            ImageType::e2D, kDepthFormat,
            VulkanHelpers::GetExtent3D(VulkanContext::swapchain->GetExtent()),
            1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::ImageLayout::eUndefined,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Image image = VulkanContext::imageManager->CreateImage(description, ImageCreateFlags::kNone);
        const vk::ImageView view = VulkanContext::imageManager->CreateView(image, ImageHelpers::kFlatDepth);

        VulkanContext::device->ExecuteOneTimeCommands([&image](vk::CommandBuffer commandBuffer)
            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eDepthStencilAttachmentOptimal,
                    PipelineBarrier{
                        SyncScope::kWaitForNothing,
                        SyncScope::KDepthStencilAttachmentWrite
                    }
                };

                ImageHelpers::TransitImageLayout(commandBuffer, image,
                        ImageHelpers::kFlatDepth, layoutTransition);
            });

        return std::make_pair(image, view);
    }

    std::unique_ptr<RenderPass> CreateRenderPass()
    {
        const AttachmentDescription colorAttachmentDescription{
            AttachmentDescription::Usage::eColor,
            VulkanContext::swapchain->GetFormat(),
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eColorAttachmentOptimal
        };

        const AttachmentDescription depthAttachmentDescription{
            AttachmentDescription::Usage::eDepth,
            kDepthFormat,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        };

        const RenderPassDescription description{
            vk::PipelineBindPoint::eGraphics, vk::SampleCountFlagBits::e1,
            { colorAttachmentDescription, depthAttachmentDescription }
        };

        std::unique_ptr<RenderPass> renderPass = RenderPass::Create(description, RenderPassDependencies{});

        return renderPass;
    }

    vk::Buffer CreateUniformBuffer(vk::DeviceSize size)
    {
        const BufferDescription description{
            size, vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        return VulkanContext::bufferManager->CreateBuffer(description, BufferCreateFlagBits::eStagingBuffer);
    }

    void UpdateUniformBuffer(vk::CommandBuffer commandBuffer, vk::Buffer buffer,
            const ByteView &byteView, const SyncScope &blockedScope)
    {
        VulkanContext::bufferManager->UpdateBuffer(commandBuffer, buffer, byteView);

        const BufferRange range{
            0, byteView.size
        };

        const PipelineBarrier barrier{
            SyncScope::kTransferWrite,
            blockedScope
        };

        BufferHelpers::SetupPipelineBarrier(commandBuffer, buffer, range, barrier);
    }

    std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline(const RenderPass &renderPass,
            const std::vector<vk::DescriptorSetLayout> &descriptorSetLayouts)
    {
        ShaderCompiler::Initialize();

        ShaderCache &shaderCache = GetRef(VulkanContext::shaderCache);
        const std::vector<ShaderModule> shaderModules{
            shaderCache.CreateShaderModule(vk::ShaderStageFlagBits::eVertex, Filepath("~/Shaders/Rasterize.vert"), {}),
            shaderCache.CreateShaderModule(vk::ShaderStageFlagBits::eFragment, Filepath("~/Shaders/Rasterize.frag"), {})
        };

        ShaderCompiler::Finalize();

        const VertexDescription vertexDescription{
            Vertex::kFormat, vk::VertexInputRate::eVertex
        };

        const GraphicsPipelineDescription description{
            VulkanContext::swapchain->GetExtent(), vk::PrimitiveTopology::eTriangleList,
            vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise,
            vk::SampleCountFlagBits::e1, vk::CompareOp::eLessOrEqual, shaderModules, { vertexDescription },
            { BlendMode::eDisabled }, descriptorSetLayouts, {}
        };

        return GraphicsPipeline::Create(renderPass.Get(), description);
    }
}

Rasterizer::Rasterizer(Scene &scene_, Camera &camera_)
    : scene(scene_)
    , camera(camera_)
{
    std::tie(depthAttachment.image, depthAttachment.view) = SRasterizer::CreateDepthAttachment();
    renderPass = SRasterizer::CreateRenderPass();
    framebuffers = VulkanHelpers::CreateSwapchainFramebuffers(VulkanContext::device->Get(), renderPass->Get(),
            VulkanContext::swapchain->GetExtent(), VulkanContext::swapchain->GetImageViews(), { depthAttachment.view });

    SetupGlobalData();
    SetupRenderObjects();

    graphicsPipeline = SRasterizer::CreateGraphicsPipeline(GetRef(renderPass), { globalLayout, renderObjectLayout });
}

Rasterizer::~Rasterizer()
{
    for (const auto &framebuffer : framebuffers)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }

    VulkanContext::imageManager->DestroyImage(depthAttachment.image);
}

void Rasterizer::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    const glm::mat4 viewProjMatrix = camera.GetProjectionMatrix() * camera.GetViewMatrix();
    SRasterizer::UpdateUniformBuffer(commandBuffer, globalData.viewProjBuffer,
            GetByteView(viewProjMatrix), SyncScope::kVertexShaderRead);

    ExecuteRenderPass(commandBuffer, imageIndex);
}

void Rasterizer::OnResize(const vk::Extent2D &)
{
    for (auto &framebuffer : framebuffers)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }

    VulkanContext::imageManager->DestroyImage(depthAttachment.image);

    std::tie(depthAttachment.image, depthAttachment.view) = SRasterizer::CreateDepthAttachment();
    renderPass = SRasterizer::CreateRenderPass();
    framebuffers = VulkanHelpers::CreateSwapchainFramebuffers(VulkanContext::device->Get(), renderPass->Get(),
            VulkanContext::swapchain->GetExtent(), VulkanContext::swapchain->GetImageViews(), { depthAttachment.view });
}

void Rasterizer::SetupGlobalData()
{
    const DescriptorSetDescription description{
        { vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex },
        { vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment },
    };

    globalLayout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);
    globalData.descriptorSet = VulkanContext::descriptorPool->AllocateDescriptorSet(globalLayout);
    globalData.viewProjBuffer = SRasterizer::CreateUniformBuffer(sizeof(glm::mat4));
    globalData.lightingBuffer = SRasterizer::CreateUniformBuffer(sizeof(glm::vec4));

    const vk::DescriptorBufferInfo viewProjInfo(globalData.viewProjBuffer, 0, sizeof(glm::mat4));
    const vk::DescriptorBufferInfo lightingInfo(globalData.lightingBuffer, 0, sizeof(glm::vec4));

    const DescriptorSetData descriptorSetData{
        { vk::DescriptorType::eUniformBuffer, viewProjInfo },
        { vk::DescriptorType::eUniformBuffer, lightingInfo }
    };

    VulkanContext::descriptorPool->UpdateDescriptorSet(globalData.descriptorSet, descriptorSetData, 0);

    VulkanContext::device->ExecuteOneTimeCommands(std::bind(&SRasterizer::UpdateUniformBuffer, std::placeholders::_1,
            globalData.lightingBuffer, GetByteView(SRasterizer::kLightDirection), SyncScope::kFragmentShaderRead));
}

void Rasterizer::SetupRenderObjects()
{
    const DescriptorSetDescription description{
        { vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment },
    };

    renderObjectLayout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);

    scene.ForEachNode([this](Node &node)
        {
            for (const auto &renderObject : node.renderObjects)
            {
                const vk::DescriptorSetLayout layout = renderObjectLayout;
                const vk::DescriptorSet descriptorSet = VulkanContext::descriptorPool->AllocateDescriptorSet(layout);

                const Texture &texture = renderObject->GetMaterial().baseColorTexture;

                const vk::DescriptorImageInfo textureInfo(texture.sampler, texture.view,
                        vk::ImageLayout::eShaderReadOnlyOptimal);

                const DescriptorSetData descriptorSetData{
                    { vk::DescriptorType::eCombinedImageSampler, textureInfo }
                };

                VulkanContext::descriptorPool->UpdateDescriptorSet(descriptorSet, descriptorSetData, 0);

                const RenderObjectData renderObjectDescriptors{
                    descriptorSet, nullptr, nullptr
                };

                renderObjects.emplace(renderObject.get(), renderObjectDescriptors);
            }
        });
}

void Rasterizer::ExecuteRenderPass(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Extent2D &extent = VulkanContext::swapchain->GetExtent();

    const vk::Rect2D renderArea(vk::Offset2D(0, 0), extent);

    const std::vector<vk::ClearValue> clearValues{
        SRasterizer::kClearColorValue, vk::ClearDepthStencilValue(1.0f, 0)
    };

    const vk::RenderPassBeginInfo beginInfo(renderPass->Get(), framebuffers[imageIndex],
            renderArea, static_cast<uint32_t>(clearValues.size()), clearValues.data());

    commandBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline->Get());
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            graphicsPipeline->GetLayout(), 0, { globalData.descriptorSet }, {});

    for (const auto &[object, data] : renderObjects)
    {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                graphicsPipeline->GetLayout(), 1, { data.descriptorSet }, {});

        commandBuffer.bindVertexBuffers(0, { object->GetVertexBuffer() }, { 0 });
        if (object->GetIndexType() != vk::IndexType::eNoneNV)
        {
            commandBuffer.bindIndexBuffer(object->GetIndexBuffer(), 0, object->GetIndexType());
            commandBuffer.drawIndexed(object->GetIndexCount(), 1, 0, 0, 0);
        }
        else
        {
            commandBuffer.draw(object->GetVertexCount(), 1, 0, 0);
        }
    }

    commandBuffer.endRenderPass();
}
