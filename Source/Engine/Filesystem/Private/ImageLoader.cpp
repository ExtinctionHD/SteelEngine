#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Engine/Filesystem/ImageLoader.hpp"

#include "Engine/Filesystem/Filepath.hpp"

#include "Utils/Assert.hpp"

namespace ImageLoader
{
    static vk::Format GetFormat(uint32_t channelCount, bool isHdr)
    {
        switch (channelCount)
        {
        case 1:
            return isHdr ? vk::Format::eR32Sfloat : vk::Format::eR8Unorm;
        case 2:
            return isHdr ? vk::Format::eR32G32Sfloat : vk::Format::eR8G8Unorm;
        case 3:
            return isHdr ? vk::Format::eR32G32B32Sfloat : vk::Format::eR8G8B8Unorm;
        case 4:
            return isHdr ? vk::Format::eR32G32B32A32Sfloat : vk::Format::eR8G8B8A8Unorm;
        default:
            Assert(false);
            return {};
        }
    }

    static ImageSource LoadLdrImage(const Filepath& filepath, uint32_t requiredChannelCount)
    {
        ImageSource imageSource;

        int32_t x = 0;
        int32_t y = 0;

        int32_t actualChannelCount;

        imageSource.data.data = stbi_load(filepath.GetAbsolute().c_str(), &x, &y, &actualChannelCount,
                static_cast<int32_t>(requiredChannelCount));

        if (requiredChannelCount > 0)
        {
            actualChannelCount = static_cast<int32_t>(requiredChannelCount);
        }

        const size_t pixelCount = static_cast<size_t>(x) * static_cast<size_t>(y);

        imageSource.data.size = pixelCount * static_cast<size_t>(actualChannelCount);

        imageSource.extent = vk::Extent2D(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
        imageSource.format = GetFormat(actualChannelCount, false);

        return imageSource;
    }

    static ImageSource LoadHdrImage(const Filepath& filepath, uint32_t requiredChannelCount)
    {
        ImageSource imageSource;

        int32_t x = 0;
        int32_t y = 0;

        int32_t actualChannelCount;

        DataAccess<float> hdrData;

        hdrData.data = stbi_loadf(filepath.GetAbsolute().c_str(), &x, &y, &actualChannelCount,
                static_cast<int32_t>(requiredChannelCount));

        if (requiredChannelCount > 0)
        {
            actualChannelCount = static_cast<int32_t>(requiredChannelCount);
        }

        const size_t pixelCount = static_cast<size_t>(x) * static_cast<size_t>(y);

        hdrData.size = pixelCount * static_cast<size_t>(actualChannelCount);

        imageSource.data = hdrData.GetByteAccess();

        imageSource.extent = vk::Extent2D(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
        imageSource.format = GetFormat(actualChannelCount, true);

        return imageSource;
    }

    bool IsHdrImageFile(const Filepath& filepath)
    {
        return stbi_is_hdr(filepath.GetAbsolute().c_str());
    }

    ImageSource LoadImage(const Filepath& filepath, uint32_t requiredChannelCount)
    {
        if (IsHdrImageFile(filepath))
        {
            return LoadHdrImage(filepath, requiredChannelCount);
        }

        return LoadLdrImage(filepath, requiredChannelCount);
    }

    void FreeImage(void* imageData)
    {
        stbi_image_free(imageData);
    }
}
