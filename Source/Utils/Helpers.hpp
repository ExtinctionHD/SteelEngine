#pragma once

namespace Numbers
{
    constexpr uint64_t kMaxUint = std::numeric_limits<uint64_t>::max();

    constexpr float kMili = 0.001f;
    constexpr float kMicro = 0.001f * kMili;
    constexpr float kNano = 0.001f * kMicro;

    constexpr uint32_t kKilobyte = 1024;
    constexpr uint32_t kMegabyte = 1024 * kKilobyte;
    constexpr uint32_t kGigabyte = 1024 * kMegabyte;
}

namespace Matrix4
{
    const glm::mat4 kIdentity = glm::mat4(1.0f);
}

namespace Vector3
{
    const glm::vec3 kZero = glm::vec3(0.0f, 0.0f, 0.0f);
    const glm::vec3 kX = glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 kY = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 kZ = glm::vec3(0.0f, 0.0f, 1.0f);
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

template <class T>
std::vector<T> Repeat(T value, size_t count)
{
    return std::vector<T>(count, value);
}

template <class TInst, class TFunc>
auto MakeFunction(TInst *instance, TFunc &&function)
{
    return [instance, function](auto &&... args)
        {
            return (instance->*function)(std::forward<decltype(args)>(args)...);
        };
}
