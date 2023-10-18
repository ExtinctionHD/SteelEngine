#include <cstdarg>

#include "Utils/Helpers.hpp"

namespace Details
{
    template <glm::length_t N>
    bool IsMatrixValid(const glm::mat<N, N, float, glm::defaultp>& matrix)
    {
        for (glm::length_t i = 0; i < N; ++i)
        {
            for (glm::length_t j = 0; j < N; ++j)
            {
                if (std::isnan(matrix[i][j]) || std::isinf(matrix[i][j]))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool IsQuatValid(const glm::quat& quaternion)
    {
        for (glm::length_t i = 0; i < glm::quat::length(); ++i)
        {
            if (std::isnan(quaternion[i]) || std::isinf(quaternion[i]))
            {
                return false;
            }
        }

        return true;
    }
}

LinearColor::LinearColor(const Color& color)
    : r(static_cast<float>(color.r) / 255.0f)
    , g(static_cast<float>(color.g) / 255.0f)
    , b(static_cast<float>(color.b) / 255.0f)
    , a(static_cast<float>(color.a) / 255.0f)
{}

bool LinearColor::operator==(const LinearColor& other) const
{
    return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool LinearColor::operator<(const LinearColor& other) const
{
    return std::tuple(r, g, b, a) < std::tuple(other.r, other.g, other.b, other.a);
}

bool Color::operator==(const Color& other) const
{
    return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool Color::operator<(const Color& other) const
{
    return std::tuple(r, g, b, a) < std::tuple(other.r, other.g, other.b, other.a);
}

const Color Color::kBlack(0, 0, 0);
const Color Color::kWhite(255, 255, 255);
const Color Color::kRed(255, 0, 0);
const Color Color::kGreen(0, 255, 0);
const Color Color::kBlue(0, 0, 255);
const Color Color::kYellow(255, 255, 0);
const Color Color::kCyan(0, 255, 255);
const Color Color::kMagenta(255, 0, 255);
const Color Color::kOrange(243, 156, 18);
const Color Color::kPurple(169, 7, 228);
const Color Color::kTurquoise(26, 188, 156);
const Color Color::kSilver(189, 195, 199);
const Color Color::kEmerald(46, 204, 113);
const Color Color::kTransparent(0, 0, 0, 0);

bool Matrix4::IsValid(const glm::mat4& matrix)
{
    return Details::IsMatrixValid(matrix);
}

bool Matrix3::IsValid(const glm::mat3& matrix)
{
    return Details::IsMatrixValid(matrix);
}

bool Quat::IsValid(const glm::quat& quaternion)
{
    return Details::IsQuatValid(quaternion);
}

std::string Format(const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    std::vector<char> v(1024);

    while (true)
    {
        va_list args2;
        va_copy(args2, args);

        const int32_t res = vsnprintf(v.data(), v.size(), fmt, args2);

        if ((res >= 0) && (res < static_cast<int32_t>(v.size())))
        {
            va_end(args);
            va_end(args2);

            return std::string(v.data());
        }

        size_t size;
        if (res < 0)
        {
            size = v.size() * 2;
        }
        else
        {
            size = static_cast<size_t>(res) + 1;
        }

        v.clear();
        v.resize(size);

        va_end(args2);
    }
}

Bytes GetBytes(const std::vector<ByteView>& byteViews)
{
    size_t size = 0;
    for (const auto& byteView : byteViews)
    {
        size += byteView.size;
    }

    Bytes bytes(size);

    size_t offset = 0;
    for (const auto& byteView : byteViews)
    {
        std::memcpy(bytes.data() + offset, byteView.data, byteView.size);

        offset += byteView.size;
    }

    return bytes;
}
