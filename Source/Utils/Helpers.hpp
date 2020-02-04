#pragma once

#include <memory>

template <class T>
const T &GetRef(const std::unique_ptr<T> &ptr)
{
    return *(ptr.get());
}

template <class T>
const T &GetRef(const std::shared_ptr<T> &ptr)
{
    return *(ptr.get());
}

template <class TSrc, class TDst>
std::vector<TDst> ConvertVectorType(const std::vector<TSrc> &src)
{
    return std::vector<TDst>(reinterpret_cast<const TDst *>(src.data()),
            reinterpret_cast<const TDst *>(src.data() + src.size()));
}

namespace Numbers
{
    constexpr uint32_t kKilobyte = 1024;
    constexpr uint32_t kMegabyte = 1024 * kKilobyte;
    constexpr uint32_t kGigabyte = 1024 * kMegabyte;
}
