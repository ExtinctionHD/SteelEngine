#include "Engine/Render/ForwardRenderPass.hpp"

#include "Engine/Camera.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
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

        const vk::Image image = VulkanContext::imageManager->CreateImage(
                description, ImageCreateFlags::kNone);

        const vk::ImageView view = VulkanContext::imageManager->CreateView(
                image, vk::ImageViewType::e2D, ImageHelpers::kFlatDepth);

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
            Renderer::kFinalLayout
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

    std::unique_ptr<GraphicsPipeline> CreateGraphicsPipeline(const RenderPass &renderPass,
            const std::vector<vk::DescriptorSetLayout> &descriptorSetLayouts)
    {
        const std::vector<ShaderModule> shaderModules{
            VulkanContext::shaderCache->CreateShaderModule(
                    vk::ShaderStageFlagBits::eVertex,
                    Filepath("~/Shaders/Forward/Forward.vert")),
            VulkanContext::shaderCache->CreateShaderModule(
                    vk::ShaderStageFlagBits::eFragment,
                    Filepath("~/Shaders/Forward/Forward.frag"))
        };

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

ForwardRenderPass::ForwardRenderPass(Scene &scene_, Camera &camera_)
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

ForwardRenderPass::~ForwardRenderPass()
{
    for (const auto &framebuffer : framebuffers)
    {
        VulkanContext::device->Get().destroyFramebuffer(framebuffer);
    }

    VulkanContext::imageManager->DestroyImage(depthAttachment.image);
}

void ForwardRenderPass::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
    const glm::mat4 viewProjMatrix = camera.GetProjectionMatrix() * camera.GetViewMatrix();
    BufferHelpers::UpdateBuffer(commandBuffer, globalUniforms.viewProjBuffer,
            GetByteView(viewProjMatrix), SyncScope::kVertexShaderRead);

    ExecuteRenderPass(commandBuffer, imageIndex);
}

void ForwardRenderPass::OnResize(const vk::Extent2D &)
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

    graphicsPipeline = SRasterizer::CreateGraphicsPipeline(GetRef(renderPass), { globalLayout, renderObjectLayout });
}

void ForwardRenderPass::SetupGlobalData()
{
    const DescriptorSetDescription description{
        DescriptorDescription{
            vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eFragment,
            vk::DescriptorBindingFlags()
        },
    };

    globalLayout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);
    globalUniforms.descriptorSet = VulkanContext::descriptorPool->AllocateDescriptorSets({ globalLayout }).front();
    globalUniforms.viewProjBuffer = BufferHelpers::CreateUniformBuffer(sizeof(glm::mat4));
    globalUniforms.lightingBuffer = BufferHelpers::CreateUniformBuffer(sizeof(glm::vec4));

    const DescriptorSetData descriptorSetData{
        DescriptorData{
            vk::DescriptorType::eUniformBuffer,
            DescriptorHelpers::GetInfo(globalUniforms.viewProjBuffer)
        },
        DescriptorData{
            vk::DescriptorType::eUniformBuffer,
            DescriptorHelpers::GetInfo(globalUniforms.lightingBuffer)
        }
    };

    VulkanContext::descriptorPool->UpdateDescriptorSet(globalUniforms.descriptorSet, descriptorSetData, 0);

    VulkanContext::device->ExecuteOneTimeCommands(std::bind(&BufferHelpers::UpdateBuffer, std::placeholders::_1,
            globalUniforms.lightingBuffer, GetByteView(SRasterizer::kLightDirection), SyncScope::kFragmentShaderRead));
}

void ForwardRenderPass::SetupRenderObjects()
{
    const DescriptorSetDescription description{
        DescriptorDescription{
            vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex,
            vk::DescriptorBindingFlags()
        },
        DescriptorDescription{
            vk::DescriptorType::eCombinedImageSampler, 1,
            vk::ShaderStageFlagBits::eFragment,
            vk::DescriptorBindingFlags()
        },
    };

    renderObjectLayout = VulkanContext::descriptorPool->CreateDescriptorSetLayout(description);

    scene.ForEachRenderObject(MakeFunction(&ForwardRenderPass::SetupRenderObject, this));
}

void ForwardRenderPass::SetupRenderObject(const RenderObject &renderObject, const glm::mat4 &transform)
{
    const vk::Buffer vertexBuffer = BufferHelpers::CreateVertexBuffer(
            renderObject.GetVertexCount() * RenderObject::kVertexStride);

    const vk::Buffer indexBuffer = BufferHelpers::CreateIndexBuffer(
            renderObject.GetIndexCount() * RenderObject::kIndexStride);

    const vk::DescriptorSetLayout layout = renderObjectLayout;
    const vk::DescriptorSet descriptorSet = VulkanContext::descriptorPool->AllocateDescriptorSets({ layout }).front();

    const vk::Buffer transformBuffer = BufferHelpers::CreateUniformBuffer(sizeof(glm::mat4));
    const Texture &baseColorTexture = renderObject.GetMaterial().baseColorTexture;

    const DescriptorSetData descriptorSetData{
        DescriptorData{
            vk::DescriptorType::eUniformBuffer,
            DescriptorHelpers::GetInfo(transformBuffer)
        },
        DescriptorData{
            vk::DescriptorType::eCombinedImageSampler,
            DescriptorHelpers::GetInfo(baseColorTexture)
        }
    };

    VulkanContext::descriptorPool->UpdateDescriptorSet(descriptorSet, descriptorSetData, 0);

    const RenderObjectEntry renderObjectData{
        vertexBuffer, indexBuffer,
        RenderObjectUniforms{
            descriptorSet, transformBuffer
        }
    };

    renderObjects.emplace(&renderObject, renderObjectData);

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            BufferHelpers::UpdateBuffer(commandBuffer, vertexBuffer,
                    GetByteView(renderObject.GetVertices()), SyncScope::kVerticesRead);

            BufferHelpers::UpdateBuffer(commandBuffer, indexBuffer,
                    GetByteView(renderObject.GetIndices()), SyncScope::kIndicesRead);

            BufferHelpers::UpdateBuffer(commandBuffer, transformBuffer,
                    GetByteView(transform), SyncScope::kVertexShaderRead);
        });
}

void ForwardRenderPass::ExecuteRenderPass(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
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
            graphicsPipeline->GetLayout(), 0, { globalUniforms.descriptorSet }, {});

    for (const auto &[renderObject, entry] : renderObjects)
    {
        const auto &[vertexBuffer, indexBuffer, renderObjectUniforms] = entry;

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                graphicsPipeline->GetLayout(), 1, { renderObjectUniforms.descriptorSet }, {});

        commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
        commandBuffer.bindIndexBuffer(indexBuffer, 0, RenderObject::kIndexType);

        commandBuffer.drawIndexed(renderObject->GetIndexCount(), 1, 0, 0, 0);
    }

    commandBuffer.endRenderPass();
}
