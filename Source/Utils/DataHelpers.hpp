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
        : data(data_)
        , size(size_)
    {}

    explicit DataView(const std::vector<T>& data_)
        : data(data_.data())
        , size(data_.size())
    {}

    const T* data = nullptr;
    size_t size = 0;

    const T& operator[](size_t i) const
    {
        return data[i];
    }

    const T& GetFirst() const
    {
        return data[0];
    }

    const T& GetLast() const
    {
        return data[size - 1];
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

    using Iterator = const T*;

    Iterator begin() const
    {
        return data;
    }

    Iterator end() const
    {
        return data + size;
    }

    std::reverse_iterator<Iterator> rbegin() const
    {
        return std::reverse_iterator<Iterator>(end());
    }

    std::reverse_iterator<Iterator> rend() const
    {
        return std::reverse_iterator<Iterator>(begin());
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

    T* data = nullptr;
    size_t size = 0;

    T& operator[](size_t i) const
    {
        return data[i];
    }

    T& GetFirst() const
    {
        return data[0];
    }

    T& GetLast() const
    {
        return data[size - 1];
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

    using Iterator = T*;
    using ConstIterator = const T*;

    Iterator begin()
    {
        return data;
    }

    Iterator end()
    {
        return data + size;
    }

    ConstIterator begin() const
    {
        return data;
    }

    ConstIterator end() const
    {
        return data + size;
    }

    std::reverse_iterator<Iterator> rbegin()
    {
        return std::reverse_iterator<Iterator>(end());
    }

    std::reverse_iterator<Iterator> rend()
    {
        return std::reverse_iterator<Iterator>(begin());
    }

    std::reverse_iterator<ConstIterator> rbegin() const
    {
        return std::reverse_iterator<ConstIterator>(end());
    }

    std::reverse_iterator<ConstIterator> rend() const
    {
        return std::reverse_iterator<ConstIterator>(begin());
    }
};

Bytes GetBytes(const std::vector<ByteView>& byteViews);

template <class... Types>
Bytes GetBytes(Types ... values)
{
    Bytes bytes;

    uint32_t offset = 0;
    const auto func = [&](const auto& value)
        {
            const uint32_t size = static_cast<uint32_t>(sizeof(value));

            bytes.resize(offset + size);
            std::memcpy(bytes.data() + offset, &value, size);

            offset += size;
        };

    (func(values), ...);

    return bytes;
}

template <class T>
ByteView GetByteView(const T& data)
{
    return ByteView(reinterpret_cast<const uint8_t*>(&data), sizeof(T));
}

template <class T>
ByteView GetByteView(const std::vector<T>& data)
{
    return ByteView(reinterpret_cast<const uint8_t*>(data.data()), data.size() * sizeof(T));
}

template <class T>
ByteAccess GetByteAccess(T& data)
{
    return ByteAccess(reinterpret_cast<uint8_t*>(&data), sizeof(T));
}

template <class T>
ByteAccess GetByteAccess(std::vector<T>& data)
{
    return ByteAccess(reinterpret_cast<uint8_t*>(data.data()), data.size() * sizeof(T));
}

template <class T, class U>
DataView<T> GetDataView(const U& data)
{
    static_assert(sizeof(U) % sizeof(T) == 0);

    return DataView<T>(reinterpret_cast<const T*>(&data), sizeof(U) / sizeof(T));
}

template <class T, class U>
DataAccess<T> GetDataAccess(U& data)
{
    static_assert(sizeof(U) % sizeof(T) == 0);

    return DataAccess<T>(reinterpret_cast<T*>(&data), sizeof(U) / sizeof(T));
}
