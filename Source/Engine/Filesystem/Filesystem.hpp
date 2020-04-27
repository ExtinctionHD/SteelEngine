#pragma once

#include <string>

#include "Engine/Filesystem/Filepath.hpp"

namespace Filesystem
{
    struct SelectDescription
    {
        std::string title;
        Filepath defaultPath;
        std::vector<std::string> filters;
    };

    std::optional<Filepath> SelectFile(const SelectDescription &description);

    std::string ReadFile(const Filepath &filepath);
}
