#pragma once

#include "Utils/Assert.hpp"

template<class T>
class ConsoleVariable
{
public:
    struct Description
    {
        std::optional<T> min = std::nullopt;
        std::optional<T> max = std::nullopt;
        std::string hint;
    };

    static_assert(std::is_same_v<T, int32_t> || std::is_same_v<T, float> || std::is_same_v<T, std::string>);

    static void LoadFromConfig()
    {
        // TODO https://github.com/Rookfighter/inifile-cpp
    }
    
    static ConsoleVariable<T>& Get(std::string&& key)
    {
        Assert(instances->contains(key));

        return instances->find(key)->second;
    }
    
    ConsoleVariable(std::string&& key_, T& value_, const Description& description_ = Description{})
        : key(key_)
        , value(value_)
        , description(description_)

    {
        if (!instances)
        {
            instances = std::make_unique<InstanceMap>();
        }

        Assert(!instances->contains(key));
        
        instances->emplace(key, *this);
    }

    ConsoleVariable(const ConsoleVariable<T>& other) = delete;
    ConsoleVariable(ConsoleVariable<T>&& other) = delete;

    ~ConsoleVariable()
    {
        instances->erase(key);

        if (instances->empty())
        {
            instances.reset();
        }
    }

    ConsoleVariable<T>& operator=(const ConsoleVariable<T>& other) = delete;
    ConsoleVariable<T>& operator=(ConsoleVariable<T>&& other) = delete;
    
    const T& GetValue() const { return value; }
    void SetValue(const T& value_) { value = value_; }

private:
    using InstanceMap = std::map<std::string, ConsoleVariable<T>&>;

    static std::unique_ptr<InstanceMap> instances;

    std::string key;
    T& value;

    Description description;
};

template<class T>
std::unique_ptr<typename ConsoleVariable<T>::InstanceMap> ConsoleVariable<T>::instances;

using CVarInt = ConsoleVariable<int32_t>;
using CVarFloat = ConsoleVariable<float>;
using CVarString = ConsoleVariable<std::string>;
