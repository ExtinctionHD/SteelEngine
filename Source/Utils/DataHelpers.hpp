#pragma once

template <class T>
struct DataAccess;

template <class T>
struct DataView
{
    DataView() = default;
    explicit DataView(const T* data_, size_t size_);
    explicit DataView(const std::vector<T>& data_);

    template <class TSrc>
    explicit DataView(const DataView<TSrc>& data_);

    template <class TSrc>
    explicit DataView(const std::vector<TSrc>& data_);

    template <class TSrc>
    explicit DataView(const TSrc& data_);

    const T* data = nullptr;
    size_t size = 0;

    const T& operator[](size_t i) const
    {
        return data[i];
    }

    template <class TDst>
    void CopyTo(const DataAccess<TDst>& dst) const;
};

template <class T>
DataView<T>::DataView(const T* data_, size_t size_)
    : data(data_)
    , size(size_)
{}

template <class T>
DataView<T>::DataView(const std::vector<T>& data_)
    : data(data_.data())
    , size(data_.size())
{}

template <class T>
template <class TSrc>
DataView<T>::DataView(const DataView<TSrc>& data_)
    : data(reinterpret_cast<const T*>(data_.data))
    , size(data_.size * sizeof(TSrc) / sizeof(T))
{}

template <class T>
template <class TSrc>
DataView<T>::DataView(const std::vector<TSrc>& data_)
    : data(reinterpret_cast<const T*>(data_.data()))
    , size(data_.size() * sizeof(TSrc) / sizeof(T))
{}

template <class T>
template <class TSrc>
DataView<T>::DataView(const TSrc& data_)
    : data(reinterpret_cast<const T*>(&data_))
    , size(sizeof(TSrc))
{}

template <class T>
struct DataAccess
{
    DataAccess() = default;
    explicit DataAccess(T* data_, size_t size_);
    explicit DataAccess(std::vector<T>& data_);

    template <class TSrc>
    explicit DataAccess(DataAccess<TSrc>& data_);

    template <class TSrc>
    explicit DataAccess(std::vector<TSrc>& data_);

    template <class TSrc>
    explicit DataAccess(TSrc& data_);

    T* data = nullptr;
    size_t size = 0;

    T& operator[](size_t i)
    {
        return data[i];
    }

    operator DataView<T>() const
    {
        return DataView<T>{ data, size };
    }

    template <class TDst>
    void CopyTo(const DataAccess<TDst>& dst) const;
};

template <class T>
DataAccess<T>::DataAccess(T* data_, size_t size_)
    : data(data_)
    , size(size_)
{}

template <class T>
DataAccess<T>::DataAccess(std::vector<T>& data_)
    : data(data_.data())
    , size(data_.size())
{}

template <class T>
template <class TSrc>
DataAccess<T>::DataAccess(DataAccess<TSrc>& data_)
    : data(reinterpret_cast<T*>(data_.data))
    , size(data_.size * sizeof(TSrc) / sizeof(T))
{}

template <class T>
template <class TSrc>
DataAccess<T>::DataAccess(std::vector<TSrc>& data_)
    : data(reinterpret_cast<T*>(data_.data()))
    , size(data_.size() * sizeof(TSrc) / sizeof(T))
{}

template <class T>
template <class TSrc>
DataAccess<T>::DataAccess(TSrc& data_)
    : data(reinterpret_cast<T*>(&data_))
    , size(sizeof(TSrc))
{}

template <class T>
template <class TDst>
void DataView<T>::CopyTo(const DataAccess<TDst>& dst) const
{
    std::memcpy(dst.data, data, size * sizeof(T));
}

template <class T>
template <class TDst>
void DataAccess<T>::CopyTo(const DataAccess<TDst>& dst) const
{
    std::memcpy(dst.data, data, size * sizeof(T));
}

using Bytes = std::vector<uint8_t>;
using ByteView = DataView<uint8_t>;
using ByteAccess = DataAccess<uint8_t>;
