#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Engine/Render/Vulkan/Resources/TextureCache.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"

#include "Shaders/Compute/Compute.h"

namespace STextureCache
{
    struct Pixels
    {
        uint8_t *data;
        vk::Extent2D extent;
        bool isHDR;
    };

    constexpr uint32_t kCubeFaceCount = 6;

    const Filepath kEquirectangularToCubeShaderPath("~/Shaders/Compute/EquirectangularToCube.comp");

    Pixels LoadTexture(const Filepath &filepath)
    {
        uint8_t *data;
        int32_t width, height;

        const bool isHDR = stbi_is_hdr(filepath.GetAbsolute().c_str());
        if (isHDR)
        {
            float *hdrData = stbi_loadf(filepath.GetAbsolute().c_str(), &width, &height, nullptr, STBI_rgb_alpha);
            data = reinterpret_cast<uint8_t*>(hdrData);
        }
        else
        {
            data = stbi_load(filepath.GetAbsolute().c_str(), &width, &height, nullptr, STBI_rgb_alpha);
        }
        Assert(data != nullptr);

        const vk::Extent2D extent(
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height));

        return Pixels{ data, extent, isHDR };
    }

    uint32_t CalculateMipLevelCount(const vk::Extent2D &extent)
    {
        return static_cast<uint32_t>(std::ceil(std::log2(std::max(extent.width, extent.height))));
    }

    void UpdateImage(vk::CommandBuffer commandBuffer, vk::Image image,
            const ImageDescription &description, const uint8_t *data)
    {
        const vk::ImageSubresourceRange fullImage(vk::ImageAspectFlagBits::eColor,
                0, description.mipLevelCount, 0, description.layerCount);

        const vk::ImageSubresourceLayers baseMipLevel = ImageHelpers::GetSubresourceLayers(fullImage, 0);

        const size_t size = static_cast<size_t>(ImageHelpers::CalculateBaseMipLevelSize(description));

        const ImageUpdate imageUpdate{
            baseMipLevel,
            { 0, 0, 0 },
            description.extent,
            ByteView{ data, size }
        };

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            PipelineBarrier{
                SyncScope::kWaitForNothing,
                SyncScope::kTransferWrite
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, image, fullImage, layoutTransition);

        VulkanContext::imageManager->UpdateImage(commandBuffer, image, { imageUpdate });
    }

    void TransitImageLayoutAfterMipmapsGenerating(vk::CommandBuffer commandBuffer,
            vk::Image image, const vk::ImageSubresourceRange &subresourceRange)
    {
        const vk::ImageSubresourceRange lastMipLevel(vk::ImageAspectFlagBits::eColor,
                subresourceRange.levelCount - 1, 1,
                subresourceRange.baseArrayLayer, subresourceRange.layerCount);

        const vk::ImageSubresourceRange exceptLastMipLevel(vk::ImageAspectFlagBits::eColor,
                subresourceRange.baseMipLevel, subresourceRange.levelCount - 1,
                subresourceRange.baseArrayLayer, subresourceRange.layerCount);

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            PipelineBarrier{
                SyncScope::kTransferRead,
                SyncScope::kShaderRead
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, image, exceptLastMipLevel, layoutTransition);

        const ImageLayoutTransition lastMipLevelLayoutTransition{
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            PipelineBarrier{
                SyncScope::kTransferWrite,
                SyncScope::kShaderRead
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, image, lastMipLevel, lastMipLevelLayoutTransition);
    }

    void ConvertEquirectangularToCube(const Texture &equirectangularTexture,
            vk::Image cubeImage, const vk::Extent2D &cubeImageExtent)
    {
        DescriptorPool &descriptorPool = GetRef(VulkanContext::descriptorPool);

        const DescriptorSetDescription cubeFaceSetDescription{
            DescriptorDescription{
                vk::DescriptorType::eStorageImage, 1,
                vk::ShaderStageFlagBits::eCompute,
                vk::DescriptorBindingFlagBits()
            }
        };

        const vk::DescriptorSetLayout cubeFaceLayout = descriptorPool.CreateDescriptorSetLayout(cubeFaceSetDescription);
        const std::vector<vk::DescriptorSet> cubeFacesDescriptors = descriptorPool.AllocateDescriptorSets(
                std::vector<vk::DescriptorSetLayout>(kCubeFaceCount, cubeFaceLayout));

        for (uint32_t i = 0; i < cubeFacesDescriptors.size(); ++i)
        {
            const vk::ImageSubresourceRange subresourceRange(
                    vk::ImageAspectFlagBits::eColor, 0, 1, i, 1);

            const vk::ImageView view = VulkanContext::imageManager->CreateView(cubeImage,
                    vk::ImageViewType::e2D, subresourceRange);

            const ImageInfo imageInfo{
                vk::DescriptorImageInfo(nullptr, view, vk::ImageLayout::eGeneral)
            };

            const DescriptorData descriptorData{
                vk::DescriptorType::eStorageImage, imageInfo
            };

            descriptorPool.UpdateDescriptorSet(cubeFacesDescriptors[i], { descriptorData }, 0);
        }

        const DescriptorSetDescription equirectangularSetDescription{
            DescriptorDescription{
                vk::DescriptorType::eCombinedImageSampler, 1,
                vk::ShaderStageFlagBits::eCompute,
                vk::DescriptorBindingFlagBits()
            }
        };

        const vk::DescriptorSetLayout equirectangularLayout
                = descriptorPool.CreateDescriptorSetLayout(equirectangularSetDescription);
        const vk::DescriptorSet equirectangularDescriptor
                = descriptorPool.AllocateDescriptorSets({ equirectangularLayout }).front();

        const DescriptorData equirectangularDescriptorData{
            vk::DescriptorType::eCombinedImageSampler,
            DescriptorHelpers::GetInfo(equirectangularTexture)
        };

        descriptorPool.UpdateDescriptorSet(equirectangularDescriptor, { equirectangularDescriptorData }, 0);

        const ShaderModule computeShaderModule = VulkanContext::shaderCache->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, kEquirectangularToCubeShaderPath);

        vk::PushConstantRange pushConstantRange(vk::ShaderStageFlagBits::eCompute,
                0, sizeof(vk::Extent2D) + sizeof(uint32_t));

        const ComputePipelineDescription pipelineDescription{
            cubeImageExtent, computeShaderModule,
            { cubeFaceLayout, equirectangularLayout },
            { pushConstantRange }
        };

        std::unique_ptr<ComputePipeline> computePipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                const vk::ImageSubresourceRange subresourceRange(
                        vk::ImageAspectFlagBits::eColor, 0, 1, 0, kCubeFaceCount);

                const ImageLayoutTransition toGeneralLayoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eGeneral,
                    PipelineBarrier{
                        SyncScope::kWaitForNothing,
                        SyncScope::KComputeShaderWrite
                    }
                };

                ImageHelpers::TransitImageLayout(commandBuffer, cubeImage,
                        subresourceRange, toGeneralLayoutTransition);

                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline->Get());

                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                        computePipeline->GetLayout(), 1, { equirectangularDescriptor }, {});

                commandBuffer.pushConstants(computePipeline->GetLayout(),
                        vk::ShaderStageFlagBits::eCompute, 0, sizeof(vk::Extent2D), &cubeImageExtent);

                const uint32_t groupCountX = static_cast<uint32_t>(std::ceil(
                        cubeImageExtent.width / static_cast<float>(LOCAL_SIZE_X)));
                const uint32_t groupCountY = static_cast<uint32_t>(std::ceil(
                        cubeImageExtent.width / static_cast<float>(LOCAL_SIZE_Y)));

                for (uint32_t i = 0; i < cubeFacesDescriptors.size(); ++i)
                {
                    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                            computePipeline->GetLayout(), 0, { cubeFacesDescriptors[i] }, {});

                    commandBuffer.pushConstants(computePipeline->GetLayout(),
                            vk::ShaderStageFlagBits::eCompute, sizeof(vk::Extent2D), sizeof(uint32_t), &i);

                    commandBuffer.dispatch(groupCountX, groupCountY, 1);
                }

                const ImageLayoutTransition GeneralToShaderOptimalLayoutTransition{
                    vk::ImageLayout::eGeneral,
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    PipelineBarrier{
                        SyncScope::KComputeShaderWrite,
                        SyncScope::kShaderRead
                    }
                };

                ImageHelpers::TransitImageLayout(commandBuffer, cubeImage,
                        subresourceRange, GeneralToShaderOptimalLayoutTransition);
            });
    }

    std::pair<vk::Image, vk::ImageView> CreateTexture(const Pixels &pixels)
    {
        const vk::Format format = pixels.isHDR ? vk::Format::eR32G32B32A32Sfloat : vk::Format::eR8G8B8A8Unorm;
        const vk::Extent3D extent(pixels.extent.width, pixels.extent.height, 1);
        const vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled
                | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

        const ImageDescription description{
            ImageType::e2D, format, extent,
            STextureCache::CalculateMipLevelCount(pixels.extent), 1,
            vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, usage,
            vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Image image = VulkanContext::imageManager->CreateImage(description,
                ImageCreateFlagBits::eStagingBuffer);

        const vk::ImageSubresourceRange fullImage(vk::ImageAspectFlagBits::eColor,
                0, description.mipLevelCount, 0, description.layerCount);

        const vk::ImageView view = VulkanContext::imageManager->CreateView(image, vk::ImageViewType::e2D, fullImage);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                STextureCache::UpdateImage(commandBuffer, image, description, pixels.data);

                if (description.mipLevelCount > 1)
                {
                    const vk::ImageSubresourceRange baseMipLevel(vk::ImageAspectFlagBits::eColor,
                            0, 1, 0, description.layerCount);

                    const ImageLayoutTransition dstToSrcLayoutTransition{
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eTransferSrcOptimal,
                        PipelineBarrier{
                            SyncScope::kTransferWrite,
                            SyncScope::kTransferRead
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, image, baseMipLevel, dstToSrcLayoutTransition);

                    ImageHelpers::GenerateMipmaps(commandBuffer, image, description.extent, fullImage);

                    TransitImageLayoutAfterMipmapsGenerating(commandBuffer, image, fullImage);
                }
                else
                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eShaderReadOnlyOptimal,
                        PipelineBarrier{
                            SyncScope::kTransferWrite,
                            SyncScope::kShaderRead
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, image, fullImage, layoutTransition);
                }
            });

        return std::make_pair(image, view);
    }
}

TextureCache::~TextureCache()
{
    for (auto &[description, sampler] : samplers)
    {
        VulkanContext::device->Get().destroySampler(sampler);
    }

    for (auto &[filepath, entry] : textures)
    {
        VulkanContext::imageManager->DestroyImage(entry.image);
    }
}

Texture TextureCache::GetTexture(const Filepath &filepath, const SamplerDescription &samplerDescription)
{
    TextureEntry &entry = textures[filepath];

    auto &[image, view] = entry;

    if (!image)
    {
        const STextureCache::Pixels pixels = STextureCache::LoadTexture(filepath);

        std::tie(image, view) = STextureCache::CreateTexture(pixels);

        stbi_image_free(pixels.data);
    }

    return { image, view, GetSampler(samplerDescription) };
}

vk::Sampler TextureCache::GetSampler(const SamplerDescription &description)
{
    const auto it = samplers.find(description);

    if (it != samplers.end())
    {
        return it->second;
    }

    const vk::SamplerCreateInfo createInfo({},
            description.magFilter, description.minFilter,
            description.mipmapMode, description.addressMode,
            description.addressMode, description.addressMode, 0.0f,
            description.maxAnisotropy.has_value(),
            description.maxAnisotropy.value_or(0.0f),
            false, vk::CompareOp(),
            description.minLod, description.maxLod,
            vk::BorderColor::eFloatOpaqueBlack, false);

    const auto &[result, sampler] = VulkanContext::device->Get().createSampler(createInfo);
    Assert(result == vk::Result::eSuccess);

    samplers.emplace(description, sampler);

    return sampler;
}

Texture TextureCache::CreateCubeTexture(const Texture &equirectangularTexture,
        const vk::Extent2D &extent, const SamplerDescription &samplerDescription)
{
    const vk::Format format = VulkanContext::imageManager->GetImageDescription(equirectangularTexture.image).format;
    const vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
            | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

    const ImageDescription imageDescription{
        ImageType::eCube, format,
        VulkanHelpers::GetExtent3D(extent),
        1, STextureCache::kCubeFaceCount,
        vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, usage,
        vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    const vk::Image cubeImage = VulkanContext::imageManager->CreateImage(imageDescription, ImageCreateFlags::kNone);

    STextureCache::ConvertEquirectangularToCube(equirectangularTexture, cubeImage, extent);

    const vk::ImageSubresourceRange subresourceRange(vk::ImageAspectFlagBits::eColor,
            0, 1, 0, STextureCache::kCubeFaceCount);

    const vk::ImageView cubeView = VulkanContext::imageManager->CreateView(cubeImage,
            vk::ImageViewType::eCube, subresourceRange);

    return { cubeImage, cubeView, GetSampler(samplerDescription) };
}
