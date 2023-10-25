#pragma once

#include "Utils/Assert.hpp"

template <class T>
struct DataView;

template <class T>
struct DataAccess;

using Bytes = std::vector<uint8_t>;
using ByteView = DataView<uint8_t>;
using ByteAccess = DataAccess<uint8_t>;

template <class T>
struct DataView
{
    DataView() = default;

    explicit DataView(const T* data_, size_t size_)
        : data(data_), size(size_)
    {
    }

    explicit DataView(const std::vector<T>& data_)
        : data(data_.data()), size(data_.size())
    {
    }

    const T* data = nullptr;
    size_t size = 0;

    const T& operator[](size_t i) const
    {
        return data[i];
    }

    ByteView GetByteView() const
    {
        return ByteView(reinterpret_cast<const uint8_t*>(data), size * sizeof(T));
    }

    std::vector<T> GetCopy() const
    {
        return std::vector<T>(data, data + size);
    }

    void CopyTo(const DataAccess<T>& dst) const
    {
        Assert(size <= dst.size);

        std::memcpy(dst.data, data, size * sizeof(T));
    }
};

template <class T>
struct DataAccess
{
    DataAccess() = default;

    explicit DataAccess(T* data_, size_t size_)
        : data(data_), size(size_)
    {
    }

    explicit DataAccess(std::vector<T>& data_)
        : data(data_.data()), size(data_.size())
    {
    }

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

    ByteAccess GetByteAccess() const
    {
        return ByteAccess(reinterpret_cast<uint8_t*>(data), size * sizeof(T));
    }

    std::vector<T> GetCopy() const
    {
        return std::vector<T>(data, data + size);
    }

    void CopyTo(const DataAccess<T>& dst) const
    {
        Assert(size <= dst.size);

        std::memcpy(dst.data, data, size * sizeof(T));
    }
};
