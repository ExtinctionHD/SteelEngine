#include "Engine/Filesystem/Filepath.hpp"

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

Filepath::Filepath(std::string path_)
{
    SFilesystem::FixPath(path_);
    if (path_.find("~/") == 0)
    {
        path_.replace(0, 2, SFilesystem::GetCurrentDirectory());
    }

    path = std::filesystem::path(path_);
}

std::string Filepath::GetAbsolute() const
{
    return std::filesystem::absolute(path).string();
}

std::string Filepath::GetDirectory() const
{
    return IsDirectory() ? path.string() : path.parent_path().string() + "/";
}

std::string Filepath::GetFilename() const
{
    return path.filename().string();
}

std::string Filepath::GetExtension() const
{
    return path.extension().string();
}

std::string Filepath::GetBaseName() const
{
    return path.stem().string();
}

bool Filepath::Exists() const
{
    return std::filesystem::exists(path);
}

bool Filepath::Empty() const
{
    return path.empty();
}

bool Filepath::IsDirectory() const
{
    return !Empty() && path.string().back() == '/' && std::filesystem::is_directory(path);
}

bool Filepath::Includes(const Filepath &directory) const
{
    Assert(directory.IsDirectory());
    return path.string().find(directory.path.string()) == 0;
}

bool Filepath::operator==(const Filepath &other) const
{
    return GetAbsolute() == other.GetAbsolute();
}
