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
};

struct Color;

struct LinearColor
{
    constexpr LinearColor() = default;

    constexpr LinearColor(float r_, float g_, float b_, float a_ = 1.0f)
        : r(r_)
        , g(g_)
        , b(b_)
        , a(a_)
    {}

    LinearColor(const Color& color);

    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    bool operator==(const LinearColor& other) const;
    bool operator<(const LinearColor& other) const;
};

struct Color
{
    constexpr Color() noexcept = default;

    constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
        : r(r_)
        , g(g_)
        , b(b_)
        , a(a_)
    {}

    constexpr Color(LinearColor linearColor)
        : r(static_cast<uint8_t>(std::clamp(linearColor.r * 255.0f, 0.0f, 255.0f)))
        , g(static_cast<uint8_t>(std::clamp(linearColor.g * 255.0f, 0.0f, 255.0f)))
        , b(static_cast<uint8_t>(std::clamp(linearColor.b * 255.0f, 0.0f, 255.0f)))
        , a(static_cast<uint8_t>(std::clamp(linearColor.a * 255.0f, 0.0f, 255.0f)))
    {}

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;

    bool operator==(const Color& other) const;
    bool operator<(const Color& other) const;

    static const Color kBlack;
    static const Color kWhite;
    static const Color kRed;
    static const Color kGreen;
    static const Color kBlue;
    static const Color kYellow;
    static const Color kCyan;
    static const Color kMagenta;
    static const Color kOrange;
    static const Color kPurple;
    static const Color kTurquoise;
    static const Color kSilver;
    static const Color kEmerald;
    static const Color kTransparent;
};

namespace Numbers
{
    constexpr float kPi = glm::pi<float>();
    constexpr float kTwoPi = 2.0f * kPi;
    constexpr float kInversePi = 1.0f / kPi;

    constexpr uint64_t kMaxUint = std::numeric_limits<uint64_t>::max();

    constexpr float kMili = 0.001f;
    constexpr float kMicro = 0.001f * kMili;
    constexpr float kNano = 0.001f * kMicro;

    constexpr uint32_t kKilobyte = 1024;
    constexpr uint32_t kMegabyte = 1024 * kKilobyte;
    constexpr uint32_t kGigabyte = 1024 * kMegabyte;
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

std::string Format(const char* fmt, ...);

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

Bytes GetBytes(const std::vector<ByteView>& byteViews);

template <class... Types>
Bytes GetBytes(Types ... values)
{
    Bytes bytes;

    uint32_t offset = 0;
    const auto func = [&](const auto& value)
        {
            const uint32_t size = static_cast<uint32_t>(sizeof(value));

            bytes.resize(offset + size);
            std::memcpy(bytes.data() + offset, &value, size);

            offset += size;
        };

    (func(values), ...);

    return bytes;
}

template <class T>
ByteView GetByteView(const T& data)
{
    return ByteView(reinterpret_cast<const uint8_t*>(&data), sizeof(T));
}


template <class T>
ByteView GetByteView(const std::vector<T>& data)
{
    return ByteView(reinterpret_cast<const uint8_t*>(data.data()), data.size() * sizeof(T));
}

template <class T>
ByteAccess GetByteAccess(T& data)
{
    return ByteAccess(reinterpret_cast<uint8_t*>(&data), sizeof(T));
}


template <class T>
ByteAccess GetByteAccess(std::vector<T>& data)
{
    return ByteAccess(reinterpret_cast<uint8_t*>(data.data()), data.size() * sizeof(T));
}

template <class T>
constexpr size_t GetSize()
{
    static_assert(std::is_array_v<T>);

    return std::extent_v<T>;
}
