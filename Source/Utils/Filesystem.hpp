#pragma once

#include <string>

namespace Filesystem
{
    const std::string kCurrentDirectoryAlias = "~/";

    std::string ReadFile(const std::string& filepath);
}

class Filepath
{
public:
    Filepath() = default;
    explicit Filepath(const std::string& path);

    const std::string &GetAbsolute() const { return absolute; };

    bool Exists() const;

    bool Empty() const;

    bool IsDirectory() const;

    bool Includes(const Filepath &directory) const;

    bool operator ==(const Filepath& other) const;

private:
    std::string absolute;
};

namespace std
{
    template <> struct hash<Filepath>
    {
        size_t operator()(const Filepath& x) const noexcept
        {
            return std::hash<string>()(x.GetAbsolute());
        }
    };
}
