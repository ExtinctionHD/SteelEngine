#pragma once

template<class T>
using Observer = T *;

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
T &GetRef(const std::unique_ptr<T> &ptr)
{
    return *ptr;
}

template <class T>
T &GetRef(const std::shared_ptr<T> &ptr)
{
    return *ptr;
}

template <class T>
Observer<T> GetObserver(const std::unique_ptr<T>& ptr)
{
    return ptr.get();
}

template <class T>
Observer<T> GetObserver(const std::shared_ptr<T>& ptr)
{
    return ptr.get();
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
