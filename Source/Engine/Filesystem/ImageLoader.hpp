#pragma once

#include "Utils/DataHelpers.hpp"

class Filepath;

struct ImageSource
{
    ByteAccess data;
    vk::Extent2D extent;
    vk::Format format;
};

namespace ImageLoader
{
    bool IsHdrImageFile(const Filepath& filepath);

    ImageSource LoadImage(const Filepath& filepath, uint32_t requiredChannelCount = 0);

    void FreeImage(void* imageData);
}
