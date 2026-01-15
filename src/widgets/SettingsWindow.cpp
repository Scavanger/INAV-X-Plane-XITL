#include "SettingsWindow.h"
#include <cstring>

#include <fa-solid-900.inc>
#include <IconsFontAwesome5.h>

#include "../settings/SettingNames.h"
#include "../settings/Settings.h"
#include "../core/PluginContext.h"
#include "../core/EventBus.h"
#include "../Utils.h"

std::shared_ptr<SettingsWindow> SettingsWindow::Instance = nullptr;

void SettingsWindow::loadSettings()
{
    auto setting = Plugin()->Settings();
    autoDetectFcPort = setting->GetSettingAs<bool>(SettingsSections::SECTION_GENERAL, SettingsKeys::SETTINGS_AUTODETECT_FC, true);

#if IBM
    std::string defaultComPort = "COMº";
#elif LIN 
    std::string defaultComPort = "/dev/ttyACM0";
#endif

    std::string comport = setting->GetSettingAs<std::string>(SettingsSections::SECTION_GENERAL, SettingsKeys::SETTINGS_COM_PORT, defaultComPort);
    for (int i = 0; i < MAX_SERIAL_PORTS_WIN; ++i)
    {
        if (comport == serialPort[i])
        {
            hitlComPort = i;
            break;
        }
    }

    const char *ip = const_cast<char *>(setting->GetSettingAs<std::string>(SettingsSections::SECTION_GENERAL, SettingsKeys::SETTINGS_SITL_IP, "127.0.0.1").c_str());
    std::strncpy(sitlIpAddress, ip, sizeof(sitlIpAddress) - 1);
    sitlIpAddress[sizeof(sitlIpAddress) - 1] = '\0';

    sitlPortIndex = setting->GetSettingAs<int>(SettingsSections::SECTION_GENERAL, SettingsKeys::SETTINGS_SITL_PORT, SITL_FIRST_PORT) - SITL_FIRST_PORT;
    restartOnAirportLoad = setting->GetSettingAs<bool>(SettingsSections::SECTION_GENERAL, SettingsKeys::SETTINGS_RESTART_ON_AIRPORT_LOAD, false);
    
    osdFilteringMode = setting->GetSettingAs<int>(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_OSD_FILTER_MODE, 1);

    std::string analogFont = setting->GetSettingAs<std::string>(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_ANALOG_OSD_FONT, "");
    auto font = std::find(analogFonts.begin(), analogFonts.end(), analogFont);
    if (font != analogFonts.end())
    {
        analogFontIndex = font - analogFonts.begin();
    }
    else
    {
        analogFontIndex = 0;
    }

    std::string hdZeroFont = setting->GetSettingAs<std::string>(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_HDZERO_OSD_FONT, "");
    font = std::find(hdZeroFonts.begin(), hdZeroFonts.end(), hdZeroFont);
    if (font != hdZeroFonts.end())
    {
        hdZeroFontIndex = font - hdZeroFonts.begin();
    }
    else
    {
        hdZeroFontIndex = 0;
    }

    std::string avatarFont = setting->GetSettingAs<std::string>(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_AVATAR_OSD_FONT, "");
    font = std::find(avatarFonts.begin(), avatarFonts.end(), avatarFont);
    if (font != avatarFonts.end())
    {
        avatarFontIndex = font - avatarFonts.begin();
    }
    else
    {
        avatarFontIndex = 0;
    }

    std::string wtfOSFont = setting->GetSettingAs<std::string>(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_WTFOS_OSD_FONT, "");
    font = std::find(wtfOSFonts.begin(), wtfOSFonts.end(), wtfOSFont);
    if (font != wtfOSFonts.end())
    {
        wtfOSFontIndex = font - wtfOSFonts.begin();
    }
    else
    {
        wtfOSFontIndex = 0;
    }

    copyAttitudeFromXPlane = setting->GetSettingAs<bool>(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_ATTITUDE_COPY_FROM_XPLANE, true);
    muteBeeper = setting->GetSettingAs<bool>(SettingsSections::SECTION_GENERAL, SettingsKeys::SETTINGS_MUTE_BEEPER, true);
}

void SettingsWindow::saveSettings()
{
    auto setting = Plugin()->Settings();
    setting->SetSetting(SettingsSections::SECTION_GENERAL, SettingsKeys::SETTINGS_AUTODETECT_FC, autoDetectFcPort);

    setting->SetSetting(SettingsSections::SECTION_GENERAL, SettingsKeys::SETTINGS_COM_PORT, std::string(serialPort[hitlComPort]));

    setting->SetSetting(SettingsSections::SECTION_GENERAL, SettingsKeys::SETTINGS_SITL_IP, std::string(sitlIpAddress));
    setting->SetSetting(SettingsSections::SECTION_GENERAL, SettingsKeys::SETTINGS_SITL_PORT, SITL_FIRST_PORT + sitlPortIndex);

    setting->SetSetting(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_OSD_FILTER_MODE, osdFilteringMode);

    setting->SetSetting(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_ANALOG_OSD_FONT, std::string(analogFonts[analogFontIndex]));
    setting->SetSetting(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_HDZERO_OSD_FONT, std::string(hdZeroFonts[hdZeroFontIndex]));
    setting->SetSetting(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_AVATAR_OSD_FONT, std::string(avatarFonts[avatarFontIndex]));
    setting->SetSetting(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_WTFOS_OSD_FONT, std::string(wtfOSFonts[wtfOSFontIndex]));

    setting->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_ATTITUDE_COPY_FROM_XPLANE, copyAttitudeFromXPlane);
    setting->SetSetting(SettingsSections::SECTION_GENERAL, SettingsKeys::SETTINGS_MUTE_BEEPER, muteBeeper);
    setting->SetSetting(SettingsSections::SECTION_GENERAL, SettingsKeys::SETTINGS_RESTART_ON_AIRPORT_LOAD, restartOnAirportLoad);

    setting->save();
}

void SettingsWindow::HelpMarker(const char *desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

SettingsWindow::SettingsWindow(int left, int top, int right, int bot, XPLMWindowDecoration decoration, XPLMWindowLayer layer) : ImgWindow(left, top, right, bot, decoration, layer)
{
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr; // Disable imgui ini file

    SetWindowTitle("INAV-X-Plane-XITL Settings");
    SetVisible(true);
#if IBM
    for (int i = 0; i < MAX_SERIAL_PORTS_WIN; ++i)
    {

        std::string portName = "COM" + std::to_string(i + 1);
        serialPort[i] = new char[portName.size() + 1];
        std::strcpy(serialPort[i], portName.c_str());
    }
#elif LIN
    for (int i = 0; i < MAX_SERIAL_PORTS_LIN; ++i)
    {

        std::string portName = "/dev/ttyACM" + std::to_string(i);
        serialPort[i] = new char[portName.size() + 1];
        std::strcpy(serialPort[i], portName.c_str());
    }
    for (int i = 0; i < MAX_SERIAL_PORTS_LIN; ++i)
    {

        std::string portName = "/dev/ttyUSB" + std::to_string(i);
        serialPort[MAX_SERIAL_PORTS_LIN + i] = new char[portName.size() + 1];
        std::strcpy(serialPort[MAX_SERIAL_PORTS_LIN + i], portName.c_str());
    }
#endif

    for (int i = 0; i < SITL_PORT_COUNT; ++i)
    {
        int port = SITL_FIRST_PORT + i;
        std::string portName = "Port " + std::to_string(port) + " (UART " + std::to_string(i + 1) + ")";

        sitlPorts[i] = new char[portName.size() + 1];
        std::strcpy(sitlPorts[i], portName.c_str());
    }

    auto fontLoadedHandler = [this](const FontEventArg &eventArg)
    {
        std::string displayName = eventArg.fontName;
        Utils::CapitalizeFirstLetter(displayName);
        Utils::ReplaceAll(displayName, "_", " ");

        std::string type = eventArg.type;

        if (type == "analog")
        {
            analogFonts.push_back(eventArg.fontName);

            analogFontsDisplayNames.push_back(new char[displayName.size() + 1]);
            std::strcpy(const_cast<char *>(analogFontsDisplayNames.back()), displayName.c_str());
        }
        else if (type == "hdzero")
        {
            hdZeroFonts.push_back(eventArg.fontName);

            hdZeroFontsDisplayNames.push_back(new char[displayName.size() + 1]);
            std::strcpy(const_cast<char *>(hdZeroFontsDisplayNames.back()), displayName.c_str());
        }
        else if (type == "avatar")
        {
            avatarFonts.push_back(eventArg.fontName);

            avatarFontsDisplayNames.push_back(new char[displayName.size() + 1]);
            std::strcpy(const_cast<char *>(avatarFontsDisplayNames.back()), displayName.c_str());
        }
        else if (type == "wtfos")
        {
            wtfOSFonts.push_back(eventArg.fontName);

            wtfOSFontsDisplayNames.push_back(new char[displayName.size() + 1]);
            std::strcpy(const_cast<char *>(wtfOSFontsDisplayNames.back()), displayName.c_str());
        }
    };

    Plugin()->GetEventBus()->Subscribe<FontEventArg>("FontLoaded", fontLoadedHandler);
}

SettingsWindow::~SettingsWindow()
{
    Instance.reset();
}

void SettingsWindow::buildInterface()
{
    float win_width = ImGui::GetWindowWidth();

    ImGui::Text("HITL Connection");

    ImGui::Checkbox("Auto detect FC Port", &autoDetectFcPort);
    ImGui::SameLine();
    HelpMarker("If enabled, the plugin will try to automatically detect the flight controller's COM port, this may take a short moment.");

    if (autoDetectFcPort)
    {
        ImGui::BeginDisabled();
    }

    ImGui::Combo("FC COM Port", &hitlComPort, serialPort, MAX_SERIAL_PORTS_WIN);

    if (autoDetectFcPort)
    {
        ImGui::EndDisabled();
    }

    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    ImGui::Text("SITL Connection");

    hasInvalidIpAddressDecoration = false;
    if (!ipAddressValid)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        hasInvalidIpAddressDecoration = true;
    }

    if (ImGui::InputText("SITL IP Address", sitlIpAddress, 16))
    {
        ipAddressValid = Utils::ValidateIpAddress(sitlIpAddress);
    }

    if (hasInvalidIpAddressDecoration)
    {
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("Invalid IP address format");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    ImGui::Combo("SITL Port", &sitlPortIndex, sitlPorts, SITL_PORT_COUNT);

    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    ImGui::Text("OSD");
    ImGui::SameLine();
    HelpMarker("Select fonts and filtering mode for the OSD display. OSD/Font type will be automatically detected based on the setting in INAV.");

    ImGui::Combo("OSD Filtering Mode", &osdFilteringMode, OSD_FILTERING_MODES, IM_ARRAYSIZE(OSD_FILTERING_MODES));
    ImGui::SameLine();
    HelpMarker("Filtering mode for OSD textures when scaling. \"Auto\": best filtering mode will be selected automatically (\"Nearest\" for Analog, \"Linear\" for Digital OSD).");

    ImGui::Combo("Analog Font", &analogFontIndex, analogFontsDisplayNames.data(), analogFontsDisplayNames.size());
    ImGui::Combo("HDZero Font", &hdZeroFontIndex, hdZeroFontsDisplayNames.data(), hdZeroFontsDisplayNames.size());
    ImGui::Combo("Avatar / DJI O3 Font", &avatarFontIndex, avatarFontsDisplayNames.data(), avatarFontsDisplayNames.size());
    ImGui::Combo("WtfOS Font", &wtfOSFontIndex, wtfOSFontsDisplayNames.data(), wtfOSFontsDisplayNames.size());
    ImGui::SameLine();
    HelpMarker("WtfOS font is used as a standard font if no connection is etablished, e.g. for messages.");

    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    ImGui::Text("General Settings");
    ImGui::Checkbox("Copy attitude from X-Plane", &copyAttitudeFromXPlane);
    ImGui::SameLine();
    HelpMarker("If enabled, the attitude (roll, pitch, yaw) will be copied from X-Plane to INAV. Disable this if you want to use simulated sensors (Gyroscope, Accelerometer, Magnetometer).");
    ImGui::Checkbox("Mute Beeper", &muteBeeper);
    ImGui::SameLine();
    HelpMarker("If enabled, the beeper on the FC will be muted.");
    ImGui::Checkbox("Reboot INAV on X-Plane airport reload", &restartOnAirportLoad);
    ImGui::SameLine();
    HelpMarker("If enabled, INAV will be rebooted automatically when a new airport (new flight) is loaded in X-Plane.");

    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    if (ImGui::Button("OK", ImVec2(win_width * 0.25f, 30.0f)))
    {
        if (ipAddressValid)
        {
            saveSettings();
            SetVisible(false);
        }
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(win_width * 0.25f, 30.0f)))
    {
        SetVisible(false);
    }

    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::Text("INAV XITL Plugin %s by Andreas Kanzler", XITL_VERSION_STRING);
    ImGui::Text("https://github.com/Scavanger/INAV-X-Plane-XITL");
    ImGui::Text("Forked from INAV X-Plane HITL by Roman Lut");
    ImGui::Text("https://github.com/RomanLut/INAV-X-Plane-HITL");
}
