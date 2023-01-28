#pragma once

template <class T>
struct DataAccess;

template <class T>
struct DataView
{
    DataView() = default;

    explicit DataView(const T* data_, size_t size_)
        : data(data_)
        , size(size_)
    {}

    explicit DataView(const std::vector<T>& data_)
        : data(data_.data())
        , size(data_.size())
    {}

    template <class TSrc>
    explicit DataView(const DataView<TSrc>& data_)
        : data(reinterpret_cast<const T*>(data_.data))
        , size(data_.size * sizeof(TSrc) / sizeof(T))
    {}

    template <class TSrc>
    explicit DataView(const std::vector<TSrc>& data_)
        : data(reinterpret_cast<const T*>(data_.data()))
        , size(data_.size() * sizeof(TSrc) / sizeof(T))
    {}

    template <class TSrc>
    explicit DataView(const TSrc& data_)
        : data(reinterpret_cast<const T*>(&data_))
        , size(sizeof(TSrc))
    {}

    const T* data = nullptr;
    size_t size = 0;

    const T& operator[](size_t i) const
    {
        return data[i];
    }

    template <class TDst>
    void CopyTo(const DataAccess<TDst>& dst) const
    {
        std::memcpy(dst.data, data, size * sizeof(T));
    }

    std::vector<T> GetCopy() const
    {
        return std::vector<T>(data, data + size);
    }
};

template <class T>
struct DataAccess
{
    DataAccess() = default;

    explicit DataAccess(T* data_, size_t size_)
        : data(data_)
        , size(size_)
    {}

    explicit DataAccess(std::vector<T>& data_)
        : data(data_.data())
        , size(data_.size())
    {}

    template <class TSrc>
    explicit DataAccess(const DataAccess<TSrc>& data_)
        : data(reinterpret_cast<T*>(data_.data))
        , size(data_.size * sizeof(TSrc) / sizeof(T))
    {}

    template <class TSrc>
    explicit DataAccess(std::vector<TSrc>& data_)
        : data(reinterpret_cast<T*>(data_.data()))
        , size(data_.size() * sizeof(TSrc) / sizeof(T))
    {}

    template <class TSrc>
    explicit DataAccess(TSrc& data_)
        : data(reinterpret_cast<T*>(&data_))
        , size(sizeof(TSrc))
    {}

    T* data = nullptr;
    size_t size = 0;

    T& operator[](size_t i) const
    {
        return data[i];
    }

    operator DataView<T>() const
    {
        return DataView<T>{ data, size };
    }

    template <class TDst>
    void CopyTo(const DataAccess<TDst>& dst) const
    {
        std::memcpy(dst.data, data, size * sizeof(T));
    }

    std::vector<T> GetCopy() const
    {
        return std::vector<T>(data, data + size);
    }
};

using Bytes = std::vector<uint8_t>;
using ByteView = DataView<uint8_t>;
using ByteAccess = DataAccess<uint8_t>;
