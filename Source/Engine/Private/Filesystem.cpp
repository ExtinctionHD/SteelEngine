#include <fstream>
#include <sstream>
#include <filesystem>

#include "Engine/Filesystem.hpp"

#include "Utils/Assert.hpp"

namespace SFilesystem
{
    void FixPath(std::string &path)
    {
        std::replace(path.begin(), path.end(), '\\', '/');
    }

    std::string GetCurrentDirectory()
    {
        std::string currentDirectory = std::filesystem::current_path().string() + "/";
        FixPath(currentDirectory);

        return currentDirectory;
    }
}

std::string Filesystem::ReadFile(const std::string &filepath)
{
    const std::ifstream file(filepath);

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

Filepath::Filepath(const std::string &path)
    : absolute(path)
{
    SFilesystem::FixPath(absolute);
    if (absolute.find(Filesystem::kCurrentDirectoryAlias) == 0)
    {
        absolute.replace(0, Filesystem::kCurrentDirectoryAlias.size(),
                SFilesystem::GetCurrentDirectory());
    }
}

bool Filepath::Exists() const
{
    const std::filesystem::path path(absolute);
    return std::filesystem::exists(path);
}

bool Filepath::Empty() const
{
    return absolute.empty();
}

bool Filepath::IsDirectory() const
{
    const std::filesystem::path path(absolute);
    return !Empty() && absolute.back() == '/' && std::filesystem::is_directory(path);
}

bool Filepath::Includes(const Filepath &directory) const
{
    Assert(directory.IsDirectory());
    return absolute.find(directory.absolute) == 0;
}

bool Filepath::operator==(const Filepath &other) const
{
    return absolute == other.absolute;
}
