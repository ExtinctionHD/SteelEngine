#include <fstream>
#include <sstream>

#include "portable-file-dialogs.h"

#include "Engine/Filesystem/Filesystem.hpp"

std::optional<Filepath> Filesystem::SelectFile(const SelectDescription &description)
{
    pfd::open_file selectDialog(description.title,
            description.defaultPath.GetAbsolute(),
            description.filters, false);

    if (!selectDialog.result().empty())
    {
        return std::make_optional(Filepath(selectDialog.result().front()));
    }

    return std::nullopt;
}

std::string Filesystem::ReadFile(const Filepath &filepath)
{
    const std::ifstream file(filepath.GetAbsolute());

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}
