#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"

std::unique_ptr<ImageManager> ResourceContext::imageManager;
std::unique_ptr<BufferManager> ResourceContext::bufferManager;
std::unique_ptr<AccelerationStructureManager> ResourceContext::accelerationStructureManager;

void ResourceContext::Create()
{
    imageManager = std::make_unique<ImageManager>();
    bufferManager = std::make_unique<BufferManager>();
    accelerationStructureManager = std::make_unique<AccelerationStructureManager>();

    TextureCache::Create();
}

void ResourceContext::Destroy()
{
    TextureCache::Destroy();

    imageManager.reset();
    bufferManager.reset();
    accelerationStructureManager.reset();
}

const ImageDescription& ResourceContext::GetImageDescription(vk::Image image)
{
    return imageManager->GetImageDescription(image);
}

const BufferDescription& ResourceContext::GetBufferDescription(vk::Buffer buffer)
{
    return bufferManager->GetBufferDescription(buffer);
}

vk::Image ResourceContext::CreateImage(const ImageDescription& description)
{
    return imageManager->CreateImage(description);
}

vk::ImageView ResourceContext::CreateImageView(const ImageViewDescription& description)
{
    return imageManager->CreateView(description);
}

CubeFaceViews ResourceContext::CreateImageCubeFaceViews(const vk::Image image)
{
    return imageManager->CreateCubeFaceViews(image);
}

BaseImage ResourceContext::CreateBaseImage(const ImageDescription& description)
{
    return imageManager->CreateBaseImage(description);
}

BaseImage ResourceContext::CreateCubeImage(const CubeImageDescription& description)
{
    return imageManager->CreateCubeImage(description);
}

vk::Buffer ResourceContext::CreateBuffer(const BufferDescription& description)
{
    return bufferManager->CreateBuffer(description);
}

vk::AccelerationStructureKHR ResourceContext::GenerateBlas(const BlasGeometryData& data)
{
    return accelerationStructureManager->GenerateBlas(data);
}

vk::AccelerationStructureKHR ResourceContext::CreateTlas(const TlasInstances& instances)
{
    return accelerationStructureManager->CreateTlas(instances);
}

void ResourceContext::UpdateImage(vk::CommandBuffer commandBuffer,
        vk::Image image, const ImageUpdateRegions& updateRegions)
{
    imageManager->UpdateImage(commandBuffer, image, updateRegions);
}

void ResourceContext::UpdateBuffer(vk::CommandBuffer commandBuffer,
        vk::Buffer buffer, const BufferUpdate& update)
{
    bufferManager->UpdateBuffer(commandBuffer, buffer, update);
}

void ResourceContext::ReadBuffer(vk::CommandBuffer commandBuffer,
        vk::Buffer buffer, const BufferReader& reader)
{
    bufferManager->ReadBuffer(commandBuffer, buffer, reader);
}

void ResourceContext::BuildTlas(vk::CommandBuffer commandBuffer,
        vk::AccelerationStructureKHR tlas, const TlasInstances& instances)
{
    accelerationStructureManager->BuildTlas(commandBuffer, tlas, instances);
}
