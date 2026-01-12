#include "Fonts.h"

#include <filesystem>

#include "../core/PluginContext.h"
#include "../core/EventBus.h"
#include "../settings/SettingNames.h"

#include "FontAnalog.h"
#include "FontHDZero.h"
#include "FontAvatar.h"
#include "FontWtfOS.h"
#include "../Utils.h"

namespace fs = std::filesystem;

Fonts::Fonts() : osdType(WtfOS)
{
    std::vector<fs::path> analogfontPaths = Utils::GetFontPaths("fonts/analog/", false);
    for (auto fontEntry : analogfontPaths)
    {
        if (Utils::ToLower(fontEntry.extension().string()) == ".png")
        {
            try
            {
                auto font = std::make_unique<FontAnalog>(fontEntry);
                Plugin()->GetEventBus()->Publish("FontLoaded", FontEventArg(font->getFontName(), "analog"));
                this->analogFonts.emplace(font->getFontName(), std::move(font));
            }
            catch (const std::exception &e)
            {
                Utils::LOG("Error loading analog font {}: {}", fontEntry.string(), e.what());
            }
        }
    }

    std::vector<fs::path> hdZeroFontPaths = Utils::GetFontPaths("fonts/digital/hdzero", false);
    for (auto fontEntry : hdZeroFontPaths)
    {
        if (Utils::ToLower(fontEntry.extension().string()) == ".bmp")
        {
            try
            {
                auto font = std::make_unique<FontHDZero>(fontEntry);
                Plugin()->GetEventBus()->Publish("FontLoaded", FontEventArg(font->getFontName(), "hdzero"));
                this->hdZeroFonts.emplace(font->getFontName(), std::move(font));
            }
            catch (const std::exception &e)
            {
                Utils::LOG("Error loading HDZero font {}: {}", fontEntry.string(), e.what());
            }
        }
    }

    std::vector<fs::path> avatarFontPaths = Utils::GetFontPaths("fonts/digital/avatar", false);
    for (auto fontEntry : avatarFontPaths)
    {
        if (Utils::ToLower(fontEntry.extension().string()) == ".png")
        {
            try
            {
                auto font = std::make_unique<FontAvatar>(fontEntry);
                Plugin()->GetEventBus()->Publish("FontLoaded", FontEventArg(font->getFontName(), "avatar"));
                this->avatarFonts.emplace(font->getFontName(), std::move(font));
            }
            catch (const std::exception &e)
            {
                Utils::LOG("Error loading Avatar font {}: {}", fontEntry.string(), e.what());
            }
        }
    }

    for (auto fontEntry : avatarFontPaths)
    {
        try
        {
            auto font = std::make_unique<FontWtfOS>(fontEntry);
            Plugin()->GetEventBus()->Publish("FontLoaded", FontEventArg(font->getFontName(), "wtfos"));
            this->wtfosFonts.emplace(font->getFontName(), std::move(font));
        }
        catch (const std::exception &e)
        {
            Utils::LOG("Error loading WTFOS font {}: {}", fontEntry.string(), e.what());
        }
    }

    this->analogFont = analogFonts.empty() ? nullptr : analogFonts.begin()->second.get();
    this->hdZeroFont = hdZeroFonts.empty() ? nullptr : hdZeroFonts.begin    ()->second.get();
    this->avatarFont = avatarFonts.empty() ? nullptr : avatarFonts.begin()->second.get();
    this->wtfosFont = wtfosFonts.empty() ? nullptr : wtfosFonts.begin()->second.get();

    Plugin()->GetEventBus()->Subscribe<SettingsChangedEventArg>("SettingsChanged", [this](const SettingsChangedEventArg &arg)
    {
        if (arg.sectionName == SettingsSections::SECTION_OSD)
        {
            if (arg.settingName == SettingsKeys::SETTINGS_ANALOG_OSD_FONT)
            {
                std::string fontName = arg.getValueAs<std::string>("");

                if (this->analogFont == nullptr)
                {
                    return;
                }

                if (fontName.empty() || fontName == this->analogFont->getFontName())
                {
                    return;
                }   
                auto it = this->analogFonts.find(fontName);
                if (it != this->analogFonts.end())
                {
                    this->analogFont = it->second.get();
                }

                if ((this->osdType == AnalogPAL || this->osdType == AnalogNTSC))
                {
                    Plugin()->GetEventBus()->Publish("FontChanged");
                }
            }
            else if (arg.settingName == SettingsKeys::SETTINGS_HDZERO_OSD_FONT)
            {
                std::string fontName = arg.getValueAs<std::string>("");

                if (this->hdZeroFont == nullptr)
                {
                    return;
                }

                if (fontName.empty() || fontName == this->hdZeroFont ->getFontName())
                {
                    return;
                }   
                auto it = this->hdZeroFonts.find(fontName);
                if (it != this->hdZeroFonts.end())
                {
                    this->hdZeroFont = it->second.get();
                }

                if (this->osdType == HDZero)
                {
                    Plugin()->GetEventBus()->Publish("FontChanged");
                }
            }
            else if (arg.settingName == SettingsKeys::SETTINGS_AVATAR_OSD_FONT)
            {
                std::string fontName = arg.getValueAs<std::string>("");

                if (this->avatarFont == nullptr)
                {
                    return;
                }

                if (fontName.empty() || fontName == this->avatarFont->getFontName())
                {
                    return;
                }   
                auto it = this->avatarFonts.find(fontName);
                if (it != this->avatarFonts.end())
                {
                    this->avatarFont = it->second.get();
                }

                if (this->osdType == Avatar)
                {
                    Plugin()->GetEventBus()->Publish("FontChanged");
                }
            }
            else if (arg.settingName == SettingsKeys::SETTINGS_WTFOS_OSD_FONT)
            {
                std::string fontName = arg.getValueAs<std::string>("");

                if (this->wtfosFont == nullptr)
                {
                    return;
                }

                if (fontName.empty() || fontName == this->wtfosFont->getFontName())
                {
                    return;
                }
                auto it = this->wtfosFonts.find(fontName);
                if (it != this->wtfosFonts.end())
                {
                    this->wtfosFont = it->second.get();
                }

                if (this->osdType == WtfOS)
                {
                    Plugin()->GetEventBus()->Publish("FontChanged");
                }
            }
        }
    }); 
}

void Fonts::setFontTypeByOsdSize(int rows, int cols)
{
    OsdType detectedType = getFontTypeByOsdSize(rows, cols);
    this->setFontType(detectedType);
}

void Fonts::setFontType(OsdType type)
{
    if (this->osdType != type)
    {
        this->osdType = type;
        Plugin()->GetEventBus()->Publish("FontChanged");
    }
}

OsdType Fonts::getFontTypeByOsdSize(int rows, int cols)
{
    if (cols == PAL_COLS && rows == PAL_ROWS)
    {
        return OsdType::AnalogPAL;
    }
    else if (cols == NTSC_COLS && rows == NTSC_ROWS)
    {
        return OsdType::AnalogNTSC;
    }
    else if (cols == DJI_COLS && rows == DJI_ROWS)
    {
        return OsdType::WtfOS;
    }
    else if (cols == HDZERO_COLS && rows == HDZERO_ROWS)
    {
        return OsdType::HDZero;
    }
    else if (cols == AVATAR_COLS && rows == AVATAR_ROWS)
    {
        return OsdType::Avatar;
    }
    return OsdType::WtfOS;
}

OsdType Fonts::getCurrentFontType()
{
    return this->osdType;
}

FontBase *Fonts::GetCurrentFont()
{
    switch (this->osdType)
    {
    case AnalogPAL:
    case AnalogNTSC:
        return this->analogFont;
    case WtfOS:
        return this->wtfosFont;
    case HDZero:
        return this->hdZeroFont;
    case Avatar:
        return this->avatarFont;
    default:
        return nullptr;
    }
}
