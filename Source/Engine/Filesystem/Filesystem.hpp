#pragma once

#include <string>

#include "Engine/Filesystem/Filepath.hpp"

struct DialogDescription
{
    std::string title;
    Filepath defaultPath;
    std::vector<std::string> filters;
};

namespace Filesystem
{
    std::optional<Filepath> ShowOpenDialog(const DialogDescription& description);

    std::optional<Filepath> ShowSaveDialog(const DialogDescription& description);

    std::string ReadFile(const Filepath& filepath);
}
