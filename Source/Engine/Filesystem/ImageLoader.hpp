#pragma once

namespace ImageLoader
{
	bool IsHdrImage(char const* filename);

	int32_t GetDefaultImageComponentsRGBA();

	// Images should be manually freed with FreeImage after data is used
	uint8_t* LoadImage(char const* filename, int32_t& widthOut, int32_t& heightOut, int32_t imageComponents);
	float* LoadHDRImage(char const* filename, int32_t& widthOut, int32_t& heightOut, int32_t imageComponents);

	void FreeImage(void* image);
}
