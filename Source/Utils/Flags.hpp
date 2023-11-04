#pragma once

using DefaultMask = uint32_t;

template <class TBit, class TMask = DefaultMask>
class Flags
{
public:
    const static Flags<TBit, TMask> kNone;
    const static Flags<TBit, TMask> kAll;

    constexpr Flags()
        : mask(0)
    {}

    constexpr Flags(TBit bit)
        : mask(static_cast<TMask>(1) << static_cast<TMask>(bit))
    {}

    constexpr Flags(const Flags<TBit, TMask>& flags)
        : mask(flags.mask)
    {}

    constexpr explicit Flags(TMask flags)
        : mask(flags)
    {}

    constexpr auto operator<=>(const Flags<TBit, TMask>&) const = default;

    constexpr bool operator!() const
    {
        return !mask;
    }

    constexpr Flags<TBit, TMask> operator&(const Flags<TBit, TMask>& other) const
    {
        return Flags<TBit, TMask>(mask & other.mask);
    }

    constexpr Flags<TBit, TMask> operator|(const Flags<TBit, TMask>& other) const
    {
        return Flags<TBit, TMask>(mask | other.mask);
    }

    constexpr Flags<TBit, TMask> operator^(const Flags<TBit, TMask>& other) const
    {
        return Flags<TBit, TMask>(mask ^ other.mask);
    }

    constexpr Flags<TBit, TMask> operator~() const
    {
        return Flags<TBit, TMask>(~mask);
    }

    constexpr Flags<TBit, TMask>& operator=(const Flags<TBit, TMask>& other)
    {
        mask = other.mask;
        return *this;
    }

    constexpr Flags<TBit, TMask>& operator|=(const Flags<TBit, TMask>& other)
    {
        mask |= other.mask;
        return *this;
    }

    constexpr Flags<TBit, TMask>& operator&=(const Flags<TBit, TMask>& other)
    {
        mask &= other.mask;
        return *this;
    }

    constexpr Flags<TBit, TMask>& operator^=(const Flags<TBit, TMask>& other)
    {
        mask ^= other.mask;
        return *this;
    }

    constexpr explicit operator bool() const
    {
        return !!mask;
    }

    constexpr explicit operator TMask() const
    {
        return mask;
    }

private:
    TMask mask;
};

template <class TBit, class TMask>
inline const Flags<TBit, TMask> Flags<TBit, TMask>::kNone = Flags<TBit, TMask>(static_cast<TMask>(0));

template <class TBit, class TMask>
inline const Flags<TBit, TMask> Flags<TBit, TMask>::kAll = ~Flags<TBit, TMask>::kNone;

#define OVERLOAD_LOGIC_OPERATORS(TFlags, TFlagBits)         \
    inline TFlags operator|(TFlagBits bit0, TFlagBits bit1) \
    {                                                       \
        return TFlags(bit0) | bit1;                         \
    }                                                       \
    inline TFlags operator&(TFlagBits bit0, TFlagBits bit1) \
    {                                                       \
        return TFlags(bit0) & bit1;                         \
    }                                                       \
    inline TFlags operator^(TFlagBits bit0, TFlagBits bit1) \
    {                                                       \
        return TFlags(bit0) ^ bit1;                         \
    }                                                       \
    inline TFlags operator~(TFlagBits bit)                  \
    {                                                       \
        return ~(TFlags(bit));                              \
    }
