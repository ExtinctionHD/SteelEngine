#pragma once

#include "Utils/Assert.hpp"

enum class eResourceState
{
    kUninitialized,
    kMarkedForUpdate,
    kUpdated
};

template <class T>
using ResourceStorage = std::list<std::pair<T *, vk::DeviceMemory>>;

namespace ResourcesHelpers
{
    template <class T>
    auto FindByHandle(const T *handle, const ResourceStorage<T> &storage)
    {
        const auto it = std::find_if(storage.begin(), storage.end(), [&handle](const auto &entry)
            {
                return entry.first == handle;
            });

        Assert(it != storage.end());

        return it;
    }
}
