#pragma once

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
        return DataView<T>{ data, size };
    }
};

using Bytes = std::vector<uint8_t>;
using ByteView = DataView<uint8_t>;
using ByteAccess = DataAccess<uint8_t>;

template <class T>
DataView<T> GetDataView(const std::vector<T> &data)
{
    return DataView<T>{ data.data(), data.size() };
}

template <class TSrc, class TDst>
DataView<TDst> GetDataView(const std::vector<TSrc> &data)
{
    return DataView<TDst>{
            reinterpret_cast<const TDst *>(data.data()),
            data.size() * sizeof(TSrc) / sizeof(TDst)
    };
}

template <class T>
ByteView GetByteView(const std::vector<T> &data)
{
    return ByteView{
        reinterpret_cast<const uint8_t *>(data.data()),
        data.size() * sizeof(T)
    };
}

template <class TSrc, class TDst>
DataAccess<TDst> GetDataAccess(std::vector<TSrc> &data)
{
    return DataAccess<TDst>{
        reinterpret_cast<TDst *>(data.data()),
        data.size() * sizeof(TSrc) / sizeof(TDst)
    };
}

template <class T>
DataAccess<T> GetDataAccess(std::vector<T> &data)
{
    return DataAccess<T>{ data.data(), data.size() };
}

template <class T>
ByteView GetByteAccess(std::vector<T> &data)
{
    return ByteView{
        reinterpret_cast<uint8_t *>(data.data()),
        data.size() * sizeof(T)
    };
}
