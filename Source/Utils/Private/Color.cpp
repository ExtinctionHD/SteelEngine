#include "Utils/Color.hpp"

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
