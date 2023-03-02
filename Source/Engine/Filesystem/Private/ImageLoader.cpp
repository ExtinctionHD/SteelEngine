#include "Engine/Filesystem/ImageLoader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ImageLoader
{
	bool IsHdrImage(char const* filename)
	{
		return stbi_is_hdr(filename);
	}

	int32_t GetDefaultImageComponentsRGBA()
	{
		return STBI_rgb_alpha;
	}

	uint8_t* LoadImage(char const* filename, int32_t& widthOut, int32_t& heightOut, int32_t imageComponents)
	{
		return stbi_load(filename, &widthOut, &heightOut, nullptr, imageComponents);
	}

	float* LoadHDRImage(char const* filename, int32_t& widthOut, int32_t& heightOut, int32_t imageComponents)
	{
		return stbi_loadf(filename, &widthOut, &heightOut, nullptr, imageComponents);
	}

	void FreeImage(void* image)
	{
		stbi_image_free(image);
	}
}
