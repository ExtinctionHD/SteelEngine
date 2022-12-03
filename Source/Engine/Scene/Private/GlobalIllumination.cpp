#include "Engine/Scene/GlobalIllumination.hpp"

#include "Engine/Render/OcclusionRenderer.hpp"
#include "Engine/Render/ProbeRenderer.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/MeshHelpers.hpp"
#include "Engine/Scene/Scene.hpp"

#include "Utils/AABBox.hpp"
#include "Utils/TimeHelpers.hpp"

namespace Details
{
    using BBoxPredicate = std::function<bool(const AABBox&)>;

    struct BBoxInfo
    {
        AABBox bbox;
        bool containsGeometry;
    };

    struct PositionInfo
    {
        glm::vec3 position;
        bool containsGeometry;
    };

    static constexpr float kEps = 0.000001f;

    static constexpr float kMinBBoxSize = 0.4f;

    static constexpr float kBBoxExtension = 0.6f;

    static constexpr uint32_t kCoefficientCount = COEFFICIENT_COUNT;

    static const Filepath kLightVolumeShaderPath("~/Shaders/Compute/GlobalIllumination/LightVolume.comp");

    static glm::vec3 Round(const glm::vec3& value)
    {
        return glm::round(value / kEps) * kEps;
    }

    static vk::DescriptorSetLayout CreateProbeLayout()
    {
        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        return VulkanContext::descriptorPool->CreateDescriptorSetLayout({ descriptorDescription });
    }

    static vk::DescriptorSetLayout CreateCoefficientsLayout()
    {
        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eStorageBuffer,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        return VulkanContext::descriptorPool->CreateDescriptorSetLayout({ descriptorDescription });
    }

    static std::unique_ptr<ComputePipeline> CreateLightVolumePipeline(
            const std::vector<vk::DescriptorSetLayout>& layouts)
    {
        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, kLightVolumeShaderPath, {});

        const vk::PushConstantRange pushConstantRange{
            vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)
        };

        const ComputePipeline::Description pipelineDescription{
            shaderModule, layouts, { pushConstantRange }
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static AABBox GetVolumeBBox(const AABBox& sceneBBox)
    {
        AABBox bbox = sceneBBox;
        bbox.Extend(kBBoxExtension);
        return bbox;
    }

    static std::array<AABBox, 8> SplitBBox(const AABBox& bbox)
    {
        const glm::vec3 halfSize = bbox.GetSize() * 0.5f;

        std::array<AABBox, 8> bboxes;

        for (size_t i = 0; i < 8; ++i)
        {
            glm::vec3 min = bbox.GetMin();

            min.x += static_cast<bool>(i & 0b001) ? halfSize.x : 0.0f;
            min.y += static_cast<bool>(i & 0b010) ? halfSize.y : 0.0f;
            min.z += static_cast<bool>(i & 0b100) ? halfSize.z : 0.0f;

            bboxes[i] = AABBox(min, min + halfSize);
        }

        return bboxes;
    }

    static void ProcessBBox(const OcclusionRenderer& renderer,
            const AABBox& bbox, std::vector<BBoxInfo>& result)
    {
        if (renderer.ContainsGeometry(bbox))
        {
            if (bbox.GetShortestEdge() * 0.5f > kMinBBoxSize)
            {
                for (const AABBox& b : SplitBBox(bbox))
                {
                    ProcessBBox(renderer, b, result);
                }
            }
            else
            {
                result.push_back(BBoxInfo{ bbox, true });
            }
        }
        else
        {
            result.push_back(BBoxInfo{ bbox, false });
        }
    }

    static std::vector<glm::vec3> GenerateLightVolumePositions(
            const Scene* scene, const AABBox& sceneBBox)
    {
        EASY_FUNCTION()

        const OcclusionRenderer occlusionRenderer(scene);

        std::vector<BBoxInfo> bboxesInfo;
        for (const AABBox& bbox : SplitBBox(sceneBBox))
        {
            ProcessBBox(occlusionRenderer, bbox, bboxesInfo);
        }

        std::vector<PositionInfo> positionsInfo;
        for (const auto& [bbox, containsGeometry] : bboxesInfo)
        {
            const std::array<glm::vec3, 8> corners = bbox.GetCorners();

            for (const auto& corner : corners)
            {
                const glm::vec3 position = Round(corner);

                const auto pred = [&](const PositionInfo& info)
                    {
                        return info.position == position;
                    };

                const auto it = std::ranges::find_if(positionsInfo, pred);

                if (it != positionsInfo.end())
                {
                    it->containsGeometry |= containsGeometry;
                }
                else
                {
                    positionsInfo.push_back(PositionInfo{ position, containsGeometry });
                }
            }
        }

        std::vector<glm::vec3> positions;
        for (const auto& [position, containsGeometry] : positionsInfo)
        {
            if (containsGeometry)
            {
                positions.push_back(position);
            }
        }

        return positions;
    }

    static vk::Buffer CreateLightVolumeCoefficientsBuffer(uint32_t probeCount)
    {
        const uint32_t size = probeCount * kCoefficientCount * sizeof(glm::vec3);

        const BufferDescription description{
            size, vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        return VulkanContext::bufferManager->CreateBuffer(description, BufferCreateFlags::kNone);
    }

    static vk::DescriptorSet AllocateCoefficientsDescriptorSet(
            vk::DescriptorSetLayout layout, vk::Buffer buffer)
    {
        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        const DescriptorData descriptorData = DescriptorHelpers::GetStorageData(buffer);

        descriptorPool.UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return descriptorSet;
    }

    static vk::DescriptorSet AllocateProbeDescriptorSet(
            vk::DescriptorSetLayout layout, vk::ImageView probeView)
    {
        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        const DescriptorData descriptorData =
                DescriptorHelpers::GetData(RenderContext::defaultSampler, probeView);

        descriptorPool.UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return descriptorSet;
    }
}

GlobalIllumination::GlobalIllumination()
{
    probeLayout = Details::CreateProbeLayout();
    coefficientsLayout = Details::CreateCoefficientsLayout();

    lightVolumePipeline = Details::CreateLightVolumePipeline(
            { probeLayout, coefficientsLayout });
}

GlobalIllumination::~GlobalIllumination()
{
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(probeLayout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(coefficientsLayout);
}

LightVolumeComponent GlobalIllumination::GenerateLightVolume(const Scene& scene) const
{
    EASY_FUNCTION()

    const std::unique_ptr<ProbeRenderer> probeRenderer = std::make_unique<ProbeRenderer>(&scene);

    const AABBox bbox = Details::GetVolumeBBox(SceneHelpers::CalculateSceneBBox(scene));
    std::vector<glm::vec3> positions = Details::GenerateLightVolumePositions(&scene, bbox);
    const auto [tetrahedral, edgeIndices] = MeshHelpers::GenerateTetrahedral(positions);

    const vk::Buffer positionsBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eStorageBuffer, ByteView(positions));
    const vk::Buffer tetrahedralBuffer = BufferHelpers::CreateBufferWithData(
            vk::BufferUsageFlagBits::eStorageBuffer, ByteView(tetrahedral));

    const uint32_t probeCount = static_cast<uint32_t>(positions.size());
    const vk::Buffer coefficientsBuffer = Details::CreateLightVolumeCoefficientsBuffer(probeCount);

    const vk::DescriptorSet coefficientsDescriptorSet
            = Details::AllocateCoefficientsDescriptorSet(coefficientsLayout, coefficientsBuffer);

    ProgressLogger progressLogger("GlobalIllumination::GenerateLightVolume", 1.0f);

    for (size_t i = 0; i < positions.size(); ++i)
    {
        const Texture probeTexture = probeRenderer->CaptureProbe(positions[i]);

        const vk::DescriptorSet probeDescriptorSet
                = Details::AllocateProbeDescriptorSet(probeLayout, probeTexture.view);

        const std::vector<vk::DescriptorSet> descriptorSets{
            probeDescriptorSet, coefficientsDescriptorSet
        };

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                EASY_BLOCK("GlobalIllumination::ProcessProbe")

                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, lightVolumePipeline->Get());

                commandBuffer.pushConstants<uint32_t>(lightVolumePipeline->GetLayout(),
                        vk::ShaderStageFlagBits::eCompute, 0, { static_cast<uint32_t>(i) });

                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                        lightVolumePipeline->GetLayout(), 0, descriptorSets, {});

                commandBuffer.dispatch(1, 1, 1);
            });

        VulkanContext::descriptorPool->FreeDescriptorSets({ probeDescriptorSet });
        VulkanContext::textureManager->DestroyTexture(probeTexture);

        progressLogger.Log(i, positions.size());
    }

    progressLogger.End();

    return LightVolumeComponent{
        positionsBuffer, tetrahedralBuffer, coefficientsBuffer, positions, edgeIndices
    };
}
