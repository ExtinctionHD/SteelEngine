#pragma once

#include "DataHelpers.hpp"

enum class Axis
{
    eX,
    eY,
    eZ
};

struct Range
{
    uint32_t offset = 0;
    uint32_t size = 0;

    uint32_t GetBegin() const { return offset; }
    uint32_t GetEnd() const { return offset + size; }
};

namespace Metric
{
    constexpr float kMili = 0.001f;
    constexpr float kMicro = 0.001f * kMili;
    constexpr float kNano = 0.001f * kMicro;

    constexpr float kKilo = 1000.0f;
    constexpr float kMega = 1000.0f * kKilo;
    constexpr float kGiga = 1000.0f * kMega;

    constexpr uint32_t kKilobyte = 1024;
    constexpr uint32_t kMegabyte = 1024 * kKilobyte;
    constexpr uint32_t kGigabyte = 1024 * kMegabyte;
}

namespace Math
{
    constexpr float kPi = glm::pi<float>();
    constexpr float kTwoPi = 2.0f * kPi;
    constexpr float kInversePi = 1.0f / kPi;

    bool IsNearlyZero(float value);

    float GetRangePercentage(float min, float max, float value);

    float RemapValueClamped(const glm::vec2& inputRange, const glm::vec2& outputRange, float value);
}

namespace Matrix3
{
    constexpr glm::mat3 kIdentity = glm::identity<glm::mat3>();

    bool IsValid(const glm::mat3& matrix);
}

namespace Matrix4
{
    constexpr glm::mat4 kIdentity = glm::identity<glm::mat4>();

    bool IsValid(const glm::mat4& matrix);
}

namespace Quat
{
    constexpr glm::quat kIdentity = glm::identity<glm::quat>();

    bool IsValid(const glm::quat& quaternion);
}

namespace Vector2
{
    constexpr glm::vec2 kZero = glm::vec2(0.0f, 0.0f);
    constexpr glm::vec2 kUnit = glm::vec2(1.0f, 1.0f);
    constexpr glm::vec2 kX = glm::vec2(1.0f, 0.0f);
    constexpr glm::vec2 kY = glm::vec2(0.0f, 1.0f);
}

namespace Vector3
{
    constexpr glm::vec3 kZero = glm::vec3(0.0f, 0.0f, 0.0f);
    constexpr glm::vec3 kUnit = glm::vec3(1.0f, 1.0f, 1.0f);
    constexpr glm::vec3 kX = glm::vec3(1.0f, 0.0f, 0.0f);
    constexpr glm::vec3 kY = glm::vec3(0.0f, 1.0f, 0.0f);
    constexpr glm::vec3 kZ = glm::vec3(0.0f, 0.0f, 1.0f);
}

namespace Vector4
{
    constexpr glm::vec4 kZero = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    constexpr glm::vec4 kUnit = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
}

template <class T>
void CombineHash(std::size_t& s, const T& v)
{
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

template <class TSrc, class TDst>
std::vector<TDst> CopyVector(const std::vector<TSrc>& src)
{
    return std::vector<TDst>(reinterpret_cast<const TDst*>(src.data()),
            reinterpret_cast<const TDst*>(src.data() + src.size()));
}

template <class T>
std::vector<T> Repeat(T value, size_t count)
{
    return std::vector<T>(count, value);
}

template <class TInst, class TFunc>
auto MakeFunction(TInst* instance, TFunc&& function)
{
    return [instance, function](auto&&... args)
        {
            return (instance->*function)(std::forward<decltype(args)>(args)...);
        };
}

template <class T>
constexpr size_t GetSize()
{
    static_assert(std::is_array_v<T>);

    return std::extent_v<T>;
}

#define DEFINE_ARRAY_FUNCTIONS(STRUCT, ELEMENT)         \
    static constexpr uint32_t GetCount()                \
    {                                                   \
        return sizeof(STRUCT) / sizeof(ELEMENT);        \
    }                                                   \
                                                        \
    DataView<ELEMENT> GetArray() const                  \
    {                                                   \
        return GetDataView<ELEMENT, STRUCT>(*this);     \
    }                                                   \
                                                        \
    DataAccess<ELEMENT> AccessArray()                   \
    {                                                   \
        return GetDataAccess<ELEMENT, STRUCT>(*this);   \
    }
