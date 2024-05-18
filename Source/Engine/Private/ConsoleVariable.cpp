#include <inicpp.h>

#include "Engine/ConsoleVariable.hpp"

#include "Engine/Filesystem/Filepath.hpp"

namespace Details
{
    static const std::string kDefaultSection = "ConsoleVariables";

    template <class T>
    static void SaveCVarsToFile(ini::IniFile& file)
    {
        ConsoleVariable<T>::Enumerate([&](const ConsoleVariable<T>& cvar)
            {
                bool found = false;

                for (auto& [name, section] : file)
                {
                    if (const auto it = section.find(cvar.GetKey()); it != section.end())
                    {
                        auto& [key, value] = *it;

                        value = cvar.GetValue();

                        found = true;

                        break;
                    }
                }

                if (!found)
                {
                    file[Details::kDefaultSection][cvar.GetKey()] = cvar.GetValue();
                }
            });
    }
}

void CVarHelpers::LoadConfig(const Filepath& path)
{
    ini::IniFile file;
    file.load(path.GetAbsolute());

    for (const auto& [name , section] : file)
    {
        for (const auto& [key, value] : section)
        {
            if (const CVarBool* cvar = CVarBool::Find(key))
            {
                cvar->value = value.as<bool>();
                continue;
            }

            if (const CVarInt* cvar = CVarInt::Find(key))
            {
                cvar->value = value.as<int>();
                continue;
            }

            if (const CVarFloat* cvar = CVarFloat::Find(key))
            {
                cvar->value = value.as<float>();
                continue;
            }

            if (const CVarString* cvar = CVarString::Find(key))
            {
                cvar->value = value.as<std::string>();
            }
        }
    }
}

void CVarHelpers::SaveConfig(const Filepath& path)
{
    ini::IniFile file;
    file.load(path.GetAbsolute());

    Details::SaveCVarsToFile<bool>(file);

    Details::SaveCVarsToFile<int>(file);

    Details::SaveCVarsToFile<float>(file);

    Details::SaveCVarsToFile<std::string>(file);
}
