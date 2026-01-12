#pragma once

#include "../platform.h"
#include "../Utils.h"

#include <filesystem>
#include <map>

#include <mini/ini.h>

namespace fs = std::filesystem;

static const std::string IniFileName = "inavxitl.ini";

class Settings
{  
private:
    mINI::INIStructure ini;
    fs::path GetIniFileName() const;
    
    // Helper: Publiziert Settings-Änderungen als Event
    void PublishSettingsChanged(const std::string& section, const std::string& key, const std::string& value);
    
public:

    Settings();
    ~Settings() ;
    
    template <typename T>
    bool SetSetting(const std::string &section, const std::string &key, const T &value)
    {
        try 
        {  
            std::string stringValue;
            // Handle string specialization
            if constexpr (std::is_same_v<T, std::string>) {
                stringValue = value;
            } else {
                stringValue = std::to_string(value);
            }
            ini[section][key] = stringValue;
            PublishSettingsChanged(section, key, stringValue);
            return true;
        }
        catch (const std::exception& e) 
        {
            Utils::LOG("Error setting setting [{}]->[{}]", section.c_str(), key.c_str());
            return false;
        }   
    }
    
    template <typename T>
    T GetSettingAs(const std::string& section, const std::string& key, const T stdValue) 
    {
        if (ini.has(section) && ini[section].has(key)) {
            const std::string& value = ini[section][key];
            if constexpr (std::is_same_v<T, std::string>) {
                return value;
            } else if constexpr (std::is_integral_v<T>) {
                return static_cast<T>(std::stoll(value));
            } else if constexpr (std::is_floating_point_v<T>) {
                return static_cast<T>(std::stod(value));
            } else if constexpr (std::is_same_v<T, bool>) {
                return (value == "1" || value == "true" || value == "True");
            } else {
               return stdValue;
            }
        } else {
            return stdValue;
        }
    }
     
    void save();
};

