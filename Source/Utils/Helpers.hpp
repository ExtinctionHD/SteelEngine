#pragma once

template <class T>
const T &GetRef(const std::unique_ptr<T> &ptr)
{
    return *(ptr.get());
}

template <class T>
const T &GetRef(const std::shared_ptr<T> &ptr)
{
    return *(ptr.get());
}

template <class TSrc, class TDst>
std::vector<TDst> ConvertVectorType(const std::vector<TSrc> &src)
{
    return std::vector<TDst>(reinterpret_cast<const TDst*>(src.data()),
            reinterpret_cast<const TDst*>(src.data() + src.size()));
}

template <class T>
struct DataView
{
    const T *data;
    size_t size;
};

template <class T>
struct DataAccess
{
    T *data;
    size_t size;

    operator DataView<T>() const
    {
        return { data, size };
    }
};

using Bytes = std::vector<uint8_t>;
using ByteView = DataView<uint8_t>;
using ByteAccess = DataAccess<uint8_t>;

template <class T>
DataView<T> GetDataView(const std::vector<T> &data)
{
    return { data.data(), data.size() };
}

template <class TSrc, class TDst>
DataView<TDst> GetDataView(const std::vector<TSrc> &data)
{
    Assert(sizeof(TSrc) % sizeof(TDst) == 0);
    return { reinterpret_cast<const TDst*>(data.data()), data.size() * sizeof(TSrc) / sizeof(TDst) };
}

template <class T>
DataView<uint8_t> GetByteView(const std::vector<T> &data)
{
    return { reinterpret_cast<const uint8_t*>(data.data()), data.size() * sizeof(T) };
}

template <class TSrc, class TDst>
DataAccess<TDst> GetDataAccess(std::vector<TSrc> &data)
{
    Assert(sizeof(TSrc) % sizeof(TDst) == 0);
    return { reinterpret_cast<TDst*>(data.data()), data.size() * sizeof(TSrc) / sizeof(TDst) };
}

template <class T>
DataAccess<T> GetDataAccess(std::vector<T> &data)
{
    return { data.data(), data.size() };
}

template <class T>
DataAccess<uint8_t> GetByteAccess(std::vector<T>& data)
{
    return { reinterpret_cast<uint8_t*>(data.data()), data.size() * sizeof(T) };
}

namespace Numbers
{
    constexpr uint32_t kKilobyte = 1024;
    constexpr uint32_t kMegabyte = 1024 * kKilobyte;
    constexpr uint32_t kGigabyte = 1024 * kMegabyte;
}
