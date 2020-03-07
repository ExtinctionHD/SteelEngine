#pragma once

#include "Utils/Helpers.hpp"

struct MemoryBlock
{
    vk::DeviceMemory memory;
    vk::DeviceSize offset;
    vk::DeviceSize size;

    bool operator ==(const MemoryBlock& other) const;
};

namespace std
{
    template <>
    struct hash<MemoryBlock>
    {
        size_t operator()(const MemoryBlock& memoryBlock) const noexcept
        {
            size_t result = 0;

            CombineHash(result, memoryBlock.memory.operator VkDeviceMemory_T * ());
            CombineHash(result, memoryBlock.offset);
            CombineHash(result, memoryBlock.size);

            return result;
        }
    };
}