#pragma once

#include <string>
#include <map>


struct DefaultSettingKey
{
    std::string section;
    std::string value;

    DefaultSettingKey(const std::string& sec, const std::string& val)
        : section(sec), value(val) {}

};

namespace SettingsSections
{
    static const std::string SECTION_GRAPH = "graph";
    static const std::string SECTION_SIMDATA = "simdata";
    static const std::string SECTION_OSD = "osd";
    static const std::string SECTION_GENERAL = "general";
}

namespace SettingsKeys
{
    static const std::string SETTINGS_GPS_NUMSAT                = "gps_numsat";
    static const std::string SETTINGS_GPS_TIMEOUT               = "gps_timeout";
    static const std::string SETTINGS_GPS_GLITCH                = "gps_glitch";
    static const std::string SETTINGS_MAG_FAILURE               = "mag_failure";
    static const std::string SETTINGS_ATTITUDE_COPY_FROM_XPLANE = "attitude_use_sensors";
    static const std::string SETTINGS_OSD_VISIBLE               = "osd_visible";
    static const std::string SETTINGS_OSD_FILTER_MODE           = "osd_smoothed";
    static const std::string SETTINGS_BATTERY_EMULATION         = "battery_emulation";
    static const std::string SETTINGS_MUTE_BEEPER               = "mute_beeper";
    static const std::string SETTINGS_SIMULATE_PITOT            = "simulate_pitot";
    static const std::string SETTINGS_GRAPH_TYPE                = "graph_type";
    static const std::string SETTINGS_VIDEOLINK_SIMULATION      = "videolink_simulation";
    static const std::string SETTINGS_ANALOG_OSD_FONT           = "analog_osd_font";
    static const std::string SETTINGS_HDZERO_OSD_FONT           = "hdzero_osd_font";
    static const std::string SETTINGS_AVATAR_OSD_FONT        = "avatar_osd_font";
    static const std::string SETTINGS_WTFOS_OSD_FONT            = "wtfos_osd_font";
    static const std::string SETTINGS_SITL_IP                   = "sitl_ip";
    static const std::string SETTINGS_SITL_PORT                 = "sitl_port";
    static const std::string SETTINGS_AUTODETECT_FC             = "autodetect_fc";
    static const std::string SETTINGS_COM_PORT                  = "com_port";
    static const std::string SETTINGS_SIMULATE_RANGEFINDER      = "simulate_rangefinder";
    static const std::string SETTINGS_RSSI_SIMULATION           = "rssi_simulation";
    static const std::string SETTINGS_RESTART_ON_AIRPORT_LOAD     = "restart_on_plane_load";
}

namespace DefaultSetting
{
    #if IBM
    const std::string defaultComPort = "COMº";
#elif LIN 
    const std::string defaultComPort = "/dev/ttyACM0";
#endif
    
    static const std::map<std::string, DefaultSettingKey> Settings = {
        { SettingsKeys::SETTINGS_GPS_NUMSAT, DefaultSettingKey(SettingsSections::SECTION_SIMDATA, "12") },
        { SettingsKeys::SETTINGS_GPS_TIMEOUT, DefaultSettingKey(SettingsSections::SECTION_SIMDATA, "0") },
        { SettingsKeys::SETTINGS_GPS_GLITCH, DefaultSettingKey(SettingsSections::SECTION_SIMDATA, "0") },
        { SettingsKeys::SETTINGS_MAG_FAILURE, DefaultSettingKey(SettingsSections::SECTION_SIMDATA, "0") },
        { SettingsKeys::SETTINGS_ATTITUDE_COPY_FROM_XPLANE, DefaultSettingKey(SettingsSections::SECTION_SIMDATA, "0") },
        { SettingsKeys::SETTINGS_BATTERY_EMULATION, DefaultSettingKey(SettingsSections::SECTION_SIMDATA, "0") },
        { SettingsKeys::SETTINGS_MUTE_BEEPER, DefaultSettingKey(SettingsSections::SECTION_SIMDATA, "1") },
        { SettingsKeys::SETTINGS_SIMULATE_PITOT, DefaultSettingKey(SettingsSections::SECTION_SIMDATA, "1") },
        { SettingsKeys::SETTINGS_GRAPH_TYPE, DefaultSettingKey(SettingsSections::SECTION_GRAPH, "0") },
        { SettingsKeys::SETTINGS_VIDEOLINK_SIMULATION, DefaultSettingKey(SettingsSections::SECTION_OSD, "3") },
        { SettingsKeys::SETTINGS_ANALOG_OSD_FONT, DefaultSettingKey(SettingsSections::SECTION_OSD, "default")},
        { SettingsKeys::SETTINGS_AVATAR_OSD_FONT, DefaultSettingKey(SettingsSections::SECTION_OSD, "Europa")},
        { SettingsKeys::SETTINGS_WTFOS_OSD_FONT, DefaultSettingKey(SettingsSections::SECTION_OSD, "Europa")},
        { SettingsKeys::SETTINGS_HDZERO_OSD_FONT, DefaultSettingKey(SettingsSections::SECTION_OSD, "SNEAKY_FPV_INAV(6)_CONTRAX_1080_v1.1")},
        { SettingsKeys::SETTINGS_OSD_VISIBLE, DefaultSettingKey(SettingsSections::SECTION_OSD, "1") },
        { SettingsKeys::SETTINGS_OSD_FILTER_MODE, DefaultSettingKey(SettingsSections::SECTION_OSD, "0") },
        { SettingsKeys::SETTINGS_SITL_IP, DefaultSettingKey(SettingsSections::SECTION_GENERAL, "127.0.0.1")},
        { SettingsKeys::SETTINGS_SITL_PORT, DefaultSettingKey(SettingsSections::SECTION_GENERAL, "5760")},
        { SettingsKeys::SETTINGS_AUTODETECT_FC, DefaultSettingKey(SettingsSections::SECTION_GENERAL, "1")},
        { SettingsKeys::SETTINGS_COM_PORT, DefaultSettingKey(SettingsSections::SECTION_GENERAL, defaultComPort)},
        { SettingsKeys::SETTINGS_RESTART_ON_AIRPORT_LOAD, DefaultSettingKey(SettingsSections::SECTION_GENERAL, "1")},
        { SettingsKeys::SETTINGS_SIMULATE_RANGEFINDER, DefaultSettingKey(SettingsSections::SECTION_SIMDATA, "0")},
        { SettingsKeys::SETTINGS_RSSI_SIMULATION, DefaultSettingKey(SettingsSections::SECTION_SIMDATA, "-1")}
    };
}
