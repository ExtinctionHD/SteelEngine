#pragma once

namespace Numbers
{
    constexpr uint32_t kKilobyte = 1024;
    constexpr uint32_t kMegabyte = 1024 * kKilobyte;
    constexpr uint32_t kGigabyte = 1024 * kMegabyte;
}

template <class T>
T &GetRef(const std::unique_ptr<T> &ptr)
{
    return *(ptr.get());
}

template <class T>
T &GetRef(const std::shared_ptr<T> &ptr)
{
    return *(ptr.get());
}

template <class T>
void CombineHash(std::size_t &s, const T &v)
{
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

template <class TSrc, class TDst>
std::vector<TDst> CopyVector(const std::vector<TSrc> &src)
{
    return std::vector<TDst>(reinterpret_cast<const TDst*>(src.data()),
            reinterpret_cast<const TDst*>(src.data() + src.size()));
}
