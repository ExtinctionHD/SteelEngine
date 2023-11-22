#include "Engine/Filesystem/Filepath.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    static void ReplaceSlashes(std::string& path)
    {
        std::ranges::replace(path, '\\', '/');
    }

    static std::string GetCurrentDirectory()
    {
        std::string currentDirectory = std::filesystem::current_path().string();

        ReplaceSlashes(currentDirectory);

        return currentDirectory + "/";
    }

    static void ResolveHomePath(std::string& path)
    {
        if (path.find("~/") == 0)
        {
            path.replace(0, 2, GetCurrentDirectory());
        }
    }
}

Filepath::Filepath(std::string path_)
{
    Details::ReplaceSlashes(path_);

    Details::ResolveHomePath(path_);

    path = path_;
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

bool Filepath::Includes(const Filepath& directory) const
{
    Assert(directory.IsDirectory());

    return GetAbsolute().find(directory.GetAbsolute()) == 0;
}

bool Filepath::operator==(const Filepath& other) const
{
    return GetAbsolute() == other.GetAbsolute();
}

bool Filepath::operator<(const Filepath& other) const
{
    return GetAbsolute() < other.GetAbsolute();
}

Filepath Filepath::operator/(const Filepath& other) const
{
    return Filepath(path.string() + "/" + other.path.string());
}
