#pragma once

namespace Numbers
{
    constexpr uint64_t kMaxUint = std::numeric_limits<uint64_t>::max();

    constexpr uint32_t kKilobyte = 1024;
    constexpr uint32_t kMegabyte = 1024 * kKilobyte;
    constexpr uint32_t kGigabyte = 1024 * kMegabyte;
}

namespace Math
{
    constexpr float kPi = 3.14159265358979323846f;
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
    return std::vector<TDst>(reinterpret_cast<const TDst *>(src.data()),
            reinterpret_cast<const TDst *>(src.data() + src.size()));
}

template <class TFunc, class TInst>
auto MakeFunction(TFunc &&function, TInst *instance)
{
    return [function, instance](auto &&... args)
        {
            return (instance->*function)(std::forward<decltype(args)>(args)...);
        };;
}
