#pragma once

#include "Utils/Assert.hpp"
#include "Utils/Flags.hpp"

class Filepath;

template <class T>
class ConsoleVariable;
class CVarContext;

template <class T>
using CVarFunc = std::function<void(ConsoleVariable<T>&)>;

enum class CVarFlagBits
{
    eReadOnly,
};

using CVarFlags = Flags<CVarFlagBits>;

OVERLOAD_LOGIC_OPERATORS(CVarFlags, CVarFlagBits)

template <class T>
class ConsoleVariable
{
public:
    static_assert(
        std::is_same_v<T, int> ||
        std::is_same_v<T, bool> ||
        std::is_same_v<T, float> ||
        std::is_same_v<T, std::string>);

    static ConsoleVariable<T>& Get(const std::string& key)
    {
        Assert(instances && instances->contains(key));

        return instances->find(key)->second;
    }

    static ConsoleVariable<T>* Find(const std::string& key)
    {
        if (instances)
        {
            if (const auto it = instances->find(key); it != instances->end())
            {
                return &(it->second);
            }
        }

        return nullptr;
    }

    static void Enumerate(const CVarFunc<T>& func)
    {
        if (instances)
        {
            for (auto& [key, instance] : *instances)
            {
                func(instance);
            }
        }
    }

    ConsoleVariable(std::string&& key_, T& value_, CVarFlags flags_ = {},
            const CVarFunc<T>& callback_ = nullptr)
        : key(key_)
        , value(value_)
        , flags(flags_)
        , callback(callback_)
    {
        Assert(!HasFlag(CVarFlagBits::eReadOnly) || callback == nullptr);

        if (!instances)
        {
            instances = std::make_unique<std::map<std::string, ConsoleVariable<T>&>>();
        }

        Assert(!instances->contains(key));

        instances->emplace(key, *this);
    }

    ConsoleVariable(const ConsoleVariable<T>&) = delete;
    ConsoleVariable(ConsoleVariable<T>&&) = delete;

    ~ConsoleVariable()
    {
        instances->erase(key);

        if (instances->empty())
        {
            instances.reset();
        }
    }

    ConsoleVariable<T>& operator=(const ConsoleVariable<T>&) = delete;
    ConsoleVariable<T>& operator=(ConsoleVariable<T>&&) = delete;

    const std::string& GetKey() const { return key; }

    const T& GetValue() const { return value; }

    void SetValue(const T& value_)
    {
        Assert(!HasFlag(CVarFlagBits::eReadOnly));

        if (value != value_)
        {
            value = value_;

            if (callback)
            {
                callback(*this);
            }
        }
    }

    CVarFlags GetFlags() const { return flags; }

    bool HasFlag(CVarFlagBits flag) const
    {
        return !!(flags & flag);
    }

    void SetCallback(const CVarFunc<T>& callback_ = nullptr)
    {
        callback = callback_;
    }

private:
    static std::unique_ptr<std::map<std::string, ConsoleVariable<T>&>> instances;

    std::string key;
    T& value;

    CVarFlags flags;

    CVarFunc<T> callback;

    friend class CVarHelpers;
};

template <class T>
std::unique_ptr<std::map<std::string, ConsoleVariable<T>&>> ConsoleVariable<T>::instances;

using CVarInt = ConsoleVariable<int>;
using CVarBool = ConsoleVariable<bool>;
using CVarFloat = ConsoleVariable<float>;
using CVarString = ConsoleVariable<std::string>;

class CVarHelpers
{
public:
    static void LoadConfig(const Filepath& path);

    static void SaveConfig(const Filepath& path);
};
