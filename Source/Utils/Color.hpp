#pragma once

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

    constexpr LinearColor(glm::vec4 vector)
        : r(vector.x)
        , g(vector.y)
        , b(vector.z)
        , a(vector.w)
    {}

    constexpr LinearColor(glm::vec3 vector)
        : r(vector.x)
        , g(vector.y)
        , b(vector.z)
    {}

    LinearColor(const Color& color);

    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    bool operator==(const LinearColor& other) const;
    bool operator<(const LinearColor& other) const;

    constexpr operator glm::vec4() const
    {
        return glm::vec4(r, g, b, a);
    }

    constexpr operator glm::vec3() const
    {
        return glm::vec3(r, g, b);
    }
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
