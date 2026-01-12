#include "../platform.h"

#include <XPLMUtilities.h>

#include "SettingNames.h"
#include "../core/PluginContext.h"
#include "../core/EventBus.h"

#include "Settings.h"

fs::path Settings::GetIniFileName() const
{
  char iniFileName[MAX_PATH] = {0};
  XPLMGetPrefsPath(iniFileName );
  char* path = XPLMExtractFileAndPath(iniFileName);
  return fs::path(iniFileName) / IniFileName;
}

Settings::Settings()
{
    const fs::path iniPath = GetIniFileName();

    if (iniPath.empty()) {
        Utils::LOG("Settings: Could not determine INI file path.");
        return;
    }

    // Apply default settings if INI file does not exist
    if (!fs::exists(iniPath)) {
        for (const auto& defSetting : DefaultSetting::Settings) {
            ini[defSetting.second.section][defSetting.first] = defSetting.second.value;
        }
    } else {
        Utils::LOG("Settings: Loading INI file from {}", iniPath.string());
        mINI::INIFile iniFile(iniPath.string());
        iniFile.read(ini);
    }

    // Publish SettingsChangedEvent for all loaded settings
    for (const auto& section : ini)
    {
        for (const auto& keyValue : section.second)
        {
            PublishSettingsChanged(section.first, keyValue.first, keyValue.second);
        }
    }
    
}

Settings::~Settings()
{
    save();
}

void Settings::PublishSettingsChanged(const std::string& section, const std::string& key, const std::string& value)
{
    try {
        Plugin()->GetEventBus()->Publish("SettingsChanged", SettingsChangedEventArg(section, key, value));
    } catch (const std::exception& e) {
        Utils::LOG("Error publishing SettingsChanged event: {}", e.what());
    }
}

void Settings::save()
{
    fs::path iniPath = GetIniFileName();
    if (!iniPath.empty())
    {
        try 
        {  
            mINI::INIFile iniFile(iniPath.string());
            iniFile.generate(ini);
        }
        catch (const std::exception& e) 
        {
            Utils::LOG("Error saving settings: {}", e.what());
        }
    }
}
