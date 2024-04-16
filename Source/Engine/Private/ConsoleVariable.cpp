#include <inicpp.h>

#include "Engine/ConsoleVariable.hpp"

#include "Engine/Filesystem/Filepath.hpp"

namespace Details
{
    static const std::string kDefaultSection = "ConsoleVariables";

    template<class T>
    static void SaveCVarToFile(const ConsoleVariable<T>& cvar, ini::IniFile& file)
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
            if (CVarInt* cvar = CVarInt::Find(key))
            {
                cvar->SetValue(value.as<int32_t>());
            }

            if (CVarFloat* cvar = CVarFloat::Find(key))
            {
                cvar->SetValue(value.as<float>());
            }

            if (CVarString* cvar = CVarString::Find(key))
            {
                cvar->SetValue(value.as<std::string>());
            }
        }
    }
}

void CVarHelpers::SaveConfig(const Filepath& path)
{
    ini::IniFile file;
    file.load(path.GetAbsolute());
    
    CVarInt::Enumerate([&](const CVarInt& cvar)
        {
            Details::SaveCVarToFile<int32_t>(cvar, file);
        });
    
    CVarFloat::Enumerate([&](const CVarFloat& cvar)
        {
            Details::SaveCVarToFile<float>(cvar, file);
        });
    
    CVarString::Enumerate([&](const CVarString& cvar)
        {
            Details::SaveCVarToFile<std::string>(cvar, file);
        });
}
