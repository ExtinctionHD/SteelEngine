#pragma once

using DefaultMask = uint32_t;

template <class TBit, class TMask = DefaultMask>
class Flags
{
public:
    static const Flags<TBit, TMask> kNone;
    static const Flags<TBit, TMask> kAll;

    Flags()
        : mask(0)
    {}

    Flags(TBit bit)
        : mask(static_cast<TMask>(1) << static_cast<TMask>(bit))
    {}

    Flags(Flags<TBit, TMask> const &flags)
        : mask(flags.mask)
    {}

    explicit Flags(TMask flags)
        : mask(flags)
    {}

    bool operator<(Flags<TBit, TMask> const &other) const
    {
        return mask < other.mask;
    }

    bool operator<=(Flags<TBit, TMask> const &other) const
    {
        return mask <= other.mask;
    }

    bool operator>(Flags<TBit, TMask> const &other) const
    {
        return mask > other.mask;
    }

    bool operator>=(Flags<TBit, TMask> const &other) const
    {
        return mask >= other.mask;
    }

    bool operator==(Flags<TBit, TMask> const &other) const
    {
        return mask == other.mask;
    }

    bool operator!=(Flags<TBit, TMask> const &other) const
    {
        return mask != other.mask;
    }

    bool operator!() const
    {
        return !mask;
    }

    Flags<TBit, TMask> operator&(Flags<TBit, TMask> const &other) const
    {
        return Flags<TBit, TMask>(mask & other.mask);
    }

    Flags<TBit, TMask> operator|(Flags<TBit, TMask> const &other) const
    {
        return Flags<TBit, TMask>(mask | other.mask);
    }

    Flags<TBit, TMask> operator^(Flags<TBit, TMask> const &other) const
    {
        return Flags<TBit, TMask>(mask ^ other.mask);
    }

    Flags<TBit, TMask> operator~() const
    {
        return Flags<TBit, TMask>(~mask);
    }

    Flags<TBit, TMask> &operator=(Flags<TBit, TMask> const &other)
    {
        mask = other.mask;
        return *this;
    }

    Flags<TBit, TMask> &operator|=(Flags<TBit, TMask> const &other)
    {
        mask |= other.mask;
        return *this;
    }

    Flags<TBit, TMask> &operator&=(Flags<TBit, TMask> const &other)
    {
        mask &= other.mask;
        return *this;
    }

    Flags<TBit, TMask> &operator^=(Flags<TBit, TMask> const &other)
    {
        mask ^= other.mask;
        return *this;
    }

    explicit operator bool() const
    {
        return !!mask;
    }

    explicit operator TMask() const
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

#define OVERLOAD_LOGIC_OPERATORS(TFlags, TFlagBits) \
    inline TFlags operator|(TFlagBits bit0, TFlagBits bit1){ return TFlags(bit0) | bit1; } \
    inline TFlags operator&(TFlagBits bit0, TFlagBits bit1){ return TFlags(bit0) & bit1; } \
    inline TFlags operator^(TFlagBits bit0, TFlagBits bit1){ return TFlags(bit0) ^ bit1; } \
    inline TFlags operator~(TFlagBits bit){ return ~(TFlags(bit)); }
