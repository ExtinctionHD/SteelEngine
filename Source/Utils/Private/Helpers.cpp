#include <cstdarg>

#include "Utils/Helpers.hpp"

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
