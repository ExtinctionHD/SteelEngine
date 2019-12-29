#pragma once

#include <memory>

template <class T>
const T &GetRef(const std::unique_ptr<T> &ptr)
{
    return *(ptr.get());
}

template <class T>
const T& GetRef(const std::shared_ptr<T>& ptr)
{
    return *(ptr.get());
}