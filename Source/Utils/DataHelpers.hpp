#pragma once

template <class T>
struct DataView
{
    DataView() = default;
    DataView(const T* data_, size_t size_);
    DataView(const std::vector<T>& data_);

    template <class TSrc>
    DataView(const DataView<TSrc>& data_);

    template <class TSrc>
    DataView(const std::vector<TSrc>& data_);

    template <class TSrc>
    DataView(const TSrc& data_);

    const T* data = nullptr;
    size_t size = 0;
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
    DataAccess(T* data_, size_t size_);
    DataAccess(std::vector<T>& data_);

    template <class TSrc>
    DataAccess(DataAccess<TSrc>& data_);

    template <class TSrc>
    DataAccess(std::vector<TSrc>& data_);

    template <class TSrc>
    DataAccess(TSrc& data_);

    T* data = nullptr;
    size_t size = 0;

    operator DataView<T>() const
    {
        return DataView<T>{ data, size };
    }
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

using Bytes = std::vector<uint8_t>;
using ByteView = DataView<uint8_t>;
using ByteAccess = DataAccess<uint8_t>;
