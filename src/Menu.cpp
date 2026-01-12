#include "Menu.h"

#include <type_traits>

#include "core/PluginContext.h"
#include "core/EventBus.h"

#include "settings/Settings.h"
#include "settings/SettingNames.h"
#include "Utils.h"

#include <cstdint>

static constexpr int SitlPortCount = 8;
static constexpr int SitlPortBase = 5760;

enum MenuItemRefs : size_t
{

    MainMenuRef,

    // Connection Menu
    ConnectMenuRef,
    ConnectDisconnectHITLItemRef,
    ConnectDisconnectSITLItemRef,

    // Hardware Menu
    HardwareMenuRef,

    // GPS Menu
    GpsMenuRef,
    GpsFailureMenuRef,
    GpsTimeoutRef,
    GpsGlitchFreezeRef,
    GpsGlitchOffsetRef,
    GpsGlitchLinearRef,
    GpsGlitchAltitudeRef,

    // Compass Menu
    CompassMenuRef,
    MagMenuRef,
    MagNormalRef,
    MagFailureRef,

    // Attitude Menu
    AttitudeMenuRef,
    AttitudeForceRef,
    AttitudeSensorsRef,

    // OSD Menu Visibility
    OsdVisibilityMenuRef,
    OsdVisibledRef,
    OsdDisabledRef,

    // OSD Fonts Menu
    OsdFontsMenuRef,
    OsdAnalogFontsMenuRef,
    OsdDigitalFontsMenuRef,

    // OSD Rows Menu
    OsdRowsMenuRef,
    OsdRowsAutoRef,
    OsdRowsPalRef,
    OsdRowsNtscRef,

    // OSD Filtering Menu
    OsdFilteringMenuRef,
    OsdFilteringNearestRef,
    OsdFilteringLinearRef,

    // Battery Menu
    BatteryMenuRef,
    BatteryNoneRef,
    BatteryInfiniteRef,
    Battery2200LipoRef,
    Battery4400LipoRef,
    Battery5200LionRef,
    Battery10400LionRef,

    // Beeper Menu
    BeeperMenuRef,
    BeeperDefaultRef,
    BeeperMuteRef,

    // Pitot Menu
    PitotMenuRef,
    PitotNoneRef,
    PitotSimulateRef,
    PitotFailureHWRef,
    PitotFailure60Ref,

    // Rangefinder Menu
    RangefinderMenuRef,
    RangefinderNoneRef,
    RangefinderSimulateRef,
    RangefinderFailureRef,

    // Noise Menu
    NoiseMenuRef,
    NoiseNoneRef,
    Noise2KmRef,
    Noise10KmRef,
    Noise50KmRef,

    // Menu
    MapMenuRef,
    MapDownloadWaypointsRef,
    MapTeleportRef,
    ShowGraphRef,
    RebootINAVRef,
    KickStartAutolaunchRef,

    // Settings Menu
    SettingsRef,

    // RSSI Menu
    RssiMenuRef,
    RxToggleFailsafeRef,
    RssiInfiniteRef,
    Rssi2KmRef,
    Rssi5KmRef,
    Rssi10KmRef,
    Rssi50KmRef,

    // Debug Menu (only in debug builds)
#ifdef DEBUG_BUILD
    DebugMenuRef,
    DrawTestOSDRef,
    ClearOSDRef,
    MakeToastRef
#endif
};


static inline void *MakeMenuRef(const MenuItemRefs ref)
{
    return (void *)ref;
}

template <typename T>
static inline T RefToEnum(const void *itemRef)
{
    if (!std::is_enum<T>::value)
    {
        throw std::invalid_argument("T is not an enum type");
    }
    return static_cast<T>(reinterpret_cast<uintptr_t>(itemRef));
}

Menu::Menu()
{
    this->CreateMenu();

    auto eventBus = Plugin()->GetEventBus();

    auto settingsChangedHandler = [this](const SettingsChangedEventArg &eventArg)
    {
        if (eventArg.sectionName == SettingsSections::SECTION_SIMDATA)
        {
            if (eventArg.settingName == SettingsKeys::SETTINGS_GPS_NUMSAT)
            {
                int sats = eventArg.getValueAs<int>(0);
                XPLMCheckMenuItem(this->gps_fix_menu_id, this->gps_fix_0_id, sats == 0 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->gps_fix_menu_id, this->gps_fix_3_id, sats == 3 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->gps_fix_menu_id, this->gps_fix_5_id, sats == 5 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->gps_fix_menu_id, this->gps_fix_12_id, sats == 12 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
            }
            else if (eventArg.settingName == SettingsKeys::SETTINGS_GPS_TIMEOUT)
            {
                bool timeout = eventArg.getValueAs<bool>(false);
                XPLMCheckMenuItem(this->gps_failure_menu_id, this->gps_timeout_id, timeout ? xplm_Menu_Checked : xplm_Menu_Unchecked);
            }
            else if (eventArg.settingName == SettingsKeys::SETTINGS_GPS_GLITCH)
            {
                int glitchType = eventArg.getValueAs<int>(0);
                XPLMCheckMenuItem(this->gps_failure_menu_id, this->gps_freeze_id, glitchType == 1 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->gps_failure_menu_id, this->gps_offset_id, glitchType == 2 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->gps_failure_menu_id, this->gps_linear_id, glitchType == 3 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->gps_failure_menu_id, this->gps_altitude_id, glitchType == 4 ? xplm_Menu_Checked : xplm_Menu_Unchecked);;
            }
            else if (eventArg.settingName == SettingsKeys::SETTINGS_MAG_FAILURE)
            {
                bool magFailure = eventArg.getValueAs<bool>(false);
                XPLMCheckMenuItem(this->mag_menu_id, this->mag_normal_id, !magFailure ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->mag_menu_id, this->mag_failure_id, magFailure ? xplm_Menu_Checked : xplm_Menu_Unchecked);
            }
            else if (eventArg.settingName == SettingsKeys::SETTINGS_BATTERY_EMULATION)
            {
                int batEmu = eventArg.getValueAs<int>(0);
                XPLMCheckMenuItem(this->battery_menu_id, this->battery_none_id, batEmu == 0 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->battery_menu_id, this->battery_infinite_id, batEmu == 1 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->battery_menu_id, this->battery_3s_2200_id, batEmu == 2 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->battery_menu_id, this->battery_3s_4400_id, batEmu == 3 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->battery_menu_id, this->battery_3s_5200_id, batEmu == 4 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->battery_menu_id, this->battery_3s_10400_id, batEmu == 5 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
            }
            else if (eventArg.settingName == SettingsKeys::SETTINGS_SIMULATE_PITOT)
            {
                int pitotSim = eventArg.getValueAs<int>(0);
                XPLMCheckMenuItem(this->pitot_menu_id, this->pitot_none_id, pitotSim == 0 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->pitot_menu_id, this->pitot_simulate_id, pitotSim == 1 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->pitot_menu_id, this->pitot_failure_hw_id, pitotSim == 2 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->pitot_menu_id, this->pitot_failure_60_id, pitotSim == 3 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
            }
            else if (eventArg.settingName == SettingsKeys::SETTINGS_RSSI_SIMULATION)
            {
                int rangeKm = eventArg.getValueAs<int>(-1);
                XPLMCheckMenuItem(this->rssi_menu_id, this->rssi_inifinite_id, rangeKm == -1 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->rssi_menu_id, this->rssi_2km_id, rangeKm == 2 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->rssi_menu_id, this->rssi_5km_id, rangeKm == 5 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->rssi_menu_id, this->rssi_10km_id, rangeKm == 10 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->rssi_menu_id, this->rssi_50km_id, rangeKm == 50 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
            }
            else if (eventArg.settingName == SettingsKeys::SETTINGS_SIMULATE_RANGEFINDER)
            {
                int rangefinderSim = eventArg.getValueAs<int>(0);
                XPLMCheckMenuItem(this->rangefinder_menu_id, this->rangefinder_none_id, rangefinderSim == 0 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->rangefinder_menu_id, this->rangefinder_simulate_id, rangefinderSim == 1 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->rangefinder_menu_id, this->rangefinder_failure_id, rangefinderSim == 2 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
            }
        }
        else if (eventArg.sectionName == SettingsSections::SECTION_OSD)
        {
            if (eventArg.settingName == SettingsKeys::SETTINGS_OSD_VISIBLE)
            {
                bool osdVisible = eventArg.getValueAs<bool>(true);
                XPLMCheckMenuItem(this->menu_id, this->osd_visibility_id, osdVisible ? xplm_Menu_Checked : xplm_Menu_Unchecked);
            }
            else if (eventArg.settingName == SettingsKeys::SETTINGS_VIDEOLINK_SIMULATION)
            {
                int videoLink = eventArg.getValueAs<int>(0);
                XPLMCheckMenuItem(this->noise_menu_id, this->noise_none_id, videoLink == 0 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->noise_menu_id, this->noise_2KM_id, videoLink == 1 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->noise_menu_id, this->noise_10KM_id, videoLink == 2 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
                XPLMCheckMenuItem(this->noise_menu_id, this->noise_50KM_id, videoLink == 3 ? xplm_Menu_Checked : xplm_Menu_Unchecked);
            }
        }
    };

    auto connectedHandler = [this](const SimulatorConnectedEventArg &event)
    {
        if (event.status == ConnectionStatus::ConnectedHitl || event.status == ConnectionStatus::ConnectedSitl)
        {
            
            if (event.status == ConnectionStatus::ConnectedHitl)
            {
                XPLMSetMenuItemName(this->menu_id, this->connect_disconnect_hitl_id, "Disconnect from HITL (FC)", 0);
                XPLMEnableMenuItem(this->menu_id, this->connect_disconnect_sitl_id, 0);
            }
            else if (event.status == ConnectionStatus::ConnectedSitl)
            {
                XPLMSetMenuItemName(this->menu_id, this->connect_disconnect_sitl_id, "Disconnect from SITL", 0);
                XPLMEnableMenuItem(this->menu_id, this->connect_disconnect_hitl_id, 0);
            }

            XPLMEnableMenuItem(this->menu_id, this->reboot_inav_id, 1);
            XPLMEnableMenuItem(this->menu_id, this->map_download_waypoints, 1);

            XPLMEnableMenuItem(this->hitlHardware_menu_id, this->battery_id, 0);
            XPLMEnableMenuItem(this->battery_menu_id, this->battery_none_id, 0);
            XPLMEnableMenuItem(this->battery_menu_id, this->battery_infinite_id, 0);
            XPLMEnableMenuItem(this->battery_menu_id, this->battery_3s_2200_id, 0);
            XPLMEnableMenuItem(this->battery_menu_id, this->battery_3s_4400_id, 0);
            XPLMEnableMenuItem(this->battery_menu_id, this->battery_3s_5200_id, 0);
            XPLMEnableMenuItem(this->battery_menu_id, this->battery_3s_10400_id, 0);
        }
        else
        {
            XPLMSetMenuItemName(this->menu_id, this->connect_disconnect_hitl_id, "Connect to HITL (FC)", 0);
            XPLMSetMenuItemName(this->menu_id, this->connect_disconnect_sitl_id, "Connect to SITL", 0);
            XPLMEnableMenuItem(this->menu_id, this->connect_disconnect_hitl_id, 1);
            XPLMEnableMenuItem(this->menu_id, this->connect_disconnect_sitl_id, 1);
            
            XPLMEnableMenuItem(this->menu_id, this->reboot_inav_id, 0);
            
            XPLMEnableMenuItem(this->hitlHardware_menu_id, this->battery_id, 1);
            XPLMEnableMenuItem(this->battery_menu_id, this->battery_none_id, 1);
            XPLMEnableMenuItem(this->battery_menu_id, this->battery_infinite_id, 1);
            XPLMEnableMenuItem(this->battery_menu_id, this->battery_3s_2200_id, 1);
            XPLMEnableMenuItem(this->battery_menu_id, this->battery_3s_4400_id, 1);
            XPLMEnableMenuItem(this->battery_menu_id, this->battery_3s_5200_id, 1);
            XPLMEnableMenuItem(this->battery_menu_id, this->battery_3s_10400_id, 1);
        }
    };

    eventBus->Subscribe<SimulatorConnectedEventArg>("SimulatorConnected", connectedHandler);
    eventBus->Subscribe<SettingsChangedEventArg>("SettingsChanged", settingsChangedHandler);
}

Menu::~Menu()
{
    XPLMDestroyMenu(this->menu_id);
}

void Menu::CreateMenu()
{
    this->menu_container_idx = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "INAV XITL", 0, 0);

    this->menu_id = XPLMCreateMenu("INAV XITL", XPLMFindPluginsMenu(), this->menu_container_idx, StaticMenuHandler, MakeMenuRef(MainMenuRef));
    this->connect_disconnect_hitl_id = XPLMAppendMenuItem(this->menu_id, "Connect to HITL (FC)", MakeMenuRef(ConnectDisconnectHITLItemRef), 0);
    this->connect_disconnect_sitl_id = XPLMAppendMenuItem(this->menu_id, "Connect to SITL", MakeMenuRef(ConnectDisconnectSITLItemRef), 0);
    XPLMAppendMenuSeparator(this->menu_id);

    this->osd_visibility_id = XPLMAppendMenuItem(this->menu_id, "OSD Visible", MakeMenuRef(OsdVisibledRef), 0);
    this->hitlHardware_id = XPLMAppendMenuItem(this->menu_id, "Hardware", NULL, 0);
    this->hitlHardware_menu_id = XPLMCreateMenu("Hardware", this->menu_id, this->hitlHardware_id, StaticMenuHandler, MakeMenuRef(HardwareMenuRef));

    this->rssi_id = XPLMAppendMenuItem(this->hitlHardware_menu_id, "RX / RSSI", NULL, 0);
    this->rssi_menu_id = XPLMCreateMenu("RX / RSSI", this->hitlHardware_menu_id, this->rssi_id, StaticMenuHandler, MakeMenuRef(RssiMenuRef));
    
    this->rssi_inifinite_id = XPLMAppendMenuItem(this->rssi_menu_id, "Infinite Range", MakeMenuRef(RssiInfiniteRef), 0);
    this->rssi_2km_id = XPLMAppendMenuItem(this->rssi_menu_id, "2 km Range", MakeMenuRef(Rssi2KmRef), 0);
    this->rssi_5km_id = XPLMAppendMenuItem(this->rssi_menu_id, "5 km Range", MakeMenuRef(Rssi5KmRef), 0);
    this->rssi_10km_id = XPLMAppendMenuItem(this->rssi_menu_id, "10 km Range", MakeMenuRef(Rssi10KmRef), 0);
    this->rssi_50km_id = XPLMAppendMenuItem(this->rssi_menu_id, "50 km Range", MakeMenuRef(Rssi50KmRef), 0);
    XPLMAppendMenuSeparator(this->rssi_menu_id);
    this->trigger_failsafe_id = XPLMAppendMenuItem(this->rssi_menu_id, "Toggle Failsafe", MakeMenuRef(RxToggleFailsafeRef), 0);

    this->gps_fix_id = XPLMAppendMenuItem(this->hitlHardware_menu_id, "GPS Fix", NULL, 0);
    this->gps_fix_menu_id = XPLMCreateMenu("GPS Fix", this->hitlHardware_menu_id, this->gps_fix_id, StaticMenuHandler, MakeMenuRef(GpsMenuRef));
    this->gps_fix_0_id = XPLMAppendMenuItem(this->gps_fix_menu_id, "0 satellites (No fix)", reinterpret_cast<void *>(0), 0);
    this->gps_fix_3_id = XPLMAppendMenuItem(this->gps_fix_menu_id, "3 satellites (No fix)", reinterpret_cast<void *>(3), 0);
    this->gps_fix_5_id = XPLMAppendMenuItem(this->gps_fix_menu_id, "5 satellites (3D fix)", reinterpret_cast<void *>(5), 0);
    this->gps_fix_12_id = XPLMAppendMenuItem(this->gps_fix_menu_id, "12 satellites (3D fix)", reinterpret_cast<void *>(12), 0);

    this->gps_failure_id = XPLMAppendMenuItem(this->gps_fix_menu_id, "GPS Failure / Glitch", NULL, 0);
    this->gps_failure_menu_id = XPLMCreateMenu("GPS Failure / Glitch", this->gps_fix_menu_id, this->gps_failure_id, StaticMenuHandler, MakeMenuRef(GpsFailureMenuRef));
    this->gps_timeout_id = XPLMAppendMenuItem(this->gps_failure_menu_id, "[HW Failure] Sensor timeout", MakeMenuRef(GpsTimeoutRef), 0);
    this->gps_freeze_id = XPLMAppendMenuItem(this->gps_failure_menu_id, "[GPS Glitch] Freeze position", MakeMenuRef(GpsGlitchFreezeRef), 0);
    this->gps_offset_id = XPLMAppendMenuItem(this->gps_failure_menu_id, "[GPS Glitch] Apply 5km offset", MakeMenuRef(GpsGlitchOffsetRef), 0);
    this->gps_linear_id = XPLMAppendMenuItem(this->gps_failure_menu_id, "[GPS Glitch] Apply linear shift 10m/s", MakeMenuRef(GpsGlitchLinearRef), 0);
    this->gps_altitude_id = XPLMAppendMenuItem(this->gps_failure_menu_id, "[GPS Glitch] Altitude up sawtooth", MakeMenuRef(GpsGlitchAltitudeRef), 0);

    this->mag_id = XPLMAppendMenuItem(this->hitlHardware_menu_id, "Compass", NULL, 0);
    this->mag_menu_id = XPLMCreateMenu("Compass", this->hitlHardware_menu_id, this->mag_id, StaticMenuHandler, MakeMenuRef(CompassMenuRef));
    this->mag_normal_id = XPLMAppendMenuItem(this->mag_menu_id, "Normal", MakeMenuRef(MagNormalRef), 0);
    this->mag_failure_id = XPLMAppendMenuItem(this->mag_menu_id, "HW Failure", MakeMenuRef(MagFailureRef), 0);

    this->battery_id = XPLMAppendMenuItem(this->hitlHardware_menu_id, "Battery", NULL, 0);
    this->battery_menu_id = XPLMCreateMenu("Battery", this->hitlHardware_menu_id, this->battery_id, StaticMenuHandler, MakeMenuRef(BatteryMenuRef));
    this->battery_none_id = XPLMAppendMenuItem(this->battery_menu_id, "Do not simulate", MakeMenuRef(BatteryNoneRef), 0);
    this->battery_infinite_id = XPLMAppendMenuItem(this->battery_menu_id, "Infinite 3s Lion", MakeMenuRef(BatteryInfiniteRef), 0);
    this->battery_3s_2200_id = XPLMAppendMenuItem(this->battery_menu_id, "2200 mAh 3s Lipo", MakeMenuRef(Battery2200LipoRef), 0);
    this->battery_3s_4400_id = XPLMAppendMenuItem(this->battery_menu_id, "4400 mAh 3s Lipo", MakeMenuRef(Battery4400LipoRef), 0);
    this->battery_3s_5200_id = XPLMAppendMenuItem(this->battery_menu_id, "5200 mAh 3s Lion", MakeMenuRef(Battery5200LionRef), 0);
    this->battery_3s_10400_id = XPLMAppendMenuItem(this->battery_menu_id, "10400 mAh 3s Lion", MakeMenuRef(Battery10400LionRef), 0);

    this->pitot_id = XPLMAppendMenuItem(this->hitlHardware_menu_id, "Pitot", NULL, 0);
    this->pitot_menu_id = XPLMCreateMenu("Pitot", this->hitlHardware_menu_id, this->pitot_id, StaticMenuHandler, MakeMenuRef(PitotMenuRef));
    this->pitot_none_id = XPLMAppendMenuItem(this->pitot_menu_id, "Do not simulate", MakeMenuRef(PitotNoneRef), 0);
    this->pitot_simulate_id = XPLMAppendMenuItem(this->pitot_menu_id, "Simulate", MakeMenuRef(PitotSimulateRef), 0);
    this->pitot_failure_hw_id = XPLMAppendMenuItem(this->pitot_menu_id, "Simulate failure", MakeMenuRef(PitotFailureHWRef), 0);
    this->pitot_failure_60_id = XPLMAppendMenuItem(this->pitot_menu_id, "Simulate failure: stuck at 60 km/h", MakeMenuRef(PitotFailure60Ref), 0);

    this->rangefinder_id = XPLMAppendMenuItem(this->hitlHardware_menu_id, "Rangefinder", NULL, 0);
    this->rangefinder_menu_id = XPLMCreateMenu("Rangefinder", this->hitlHardware_menu_id, this->rangefinder_id, StaticMenuHandler, MakeMenuRef(RangefinderMenuRef));
    this->rangefinder_none_id = XPLMAppendMenuItem(this->rangefinder_menu_id, "Do not simulate", MakeMenuRef(RangefinderNoneRef), 0);
    this->rangefinder_simulate_id = XPLMAppendMenuItem(this->rangefinder_menu_id, "Simulate", MakeMenuRef(RangefinderSimulateRef), 0);
    this->rangefinder_failure_id = XPLMAppendMenuItem(this->rangefinder_menu_id, "Simulate failure: Stuck at 0 cm", MakeMenuRef(RangefinderFailureRef), 0);

    this->noise_id = XPLMAppendMenuItem(this->menu_id, "Analog Video", NULL, 0);
    this->noise_menu_id = XPLMCreateMenu("Video", this->menu_id, this->noise_id, StaticMenuHandler, MakeMenuRef(NoiseMenuRef));
    this->noise_none_id = XPLMAppendMenuItem(this->noise_menu_id, "No simulation", MakeMenuRef(NoiseNoneRef), 0);
    this->noise_2KM_id = XPLMAppendMenuItem(this->noise_menu_id, "Link up to 2km", MakeMenuRef(Noise2KmRef), 0);
    this->noise_10KM_id = XPLMAppendMenuItem(this->noise_menu_id, "Link up to 10km", MakeMenuRef(Noise10KmRef), 0);
    this->noise_50KM_id = XPLMAppendMenuItem(this->noise_menu_id, "Link up to 50km", MakeMenuRef(Noise50KmRef), 0);
    XPLMAppendMenuSeparator(this->menu_id);

    this->map_download_waypoints = XPLMAppendMenuItem(this->menu_id, "Download waypoints from FC", MakeMenuRef(MapDownloadWaypointsRef), 0);
    XPLMEnableMenuItem(this->menu_id, this->map_download_waypoints, 0);
    this->map_teleport = XPLMAppendMenuItem(this->menu_id, "Teleport to location (from clipboard)", MakeMenuRef(MapTeleportRef), 0);
    this->show_graph_id = XPLMAppendMenuItem(this->menu_id, "Show Graph", MakeMenuRef(ShowGraphRef), 0);
    this->reboot_inav_id = XPLMAppendMenuItem(this->menu_id, "Reboot INAV", MakeMenuRef(RebootINAVRef), 0);
    this->kickstart_autolaunch_id = XPLMAppendMenuItem(this->menu_id, "Kickstart Autolaunch", MakeMenuRef(KickStartAutolaunchRef), 0);
    XPLMEnableMenuItem(this->menu_id, this->reboot_inav_id, 0);
    XPLMAppendMenuSeparator(this->menu_id);

    XPLMAppendMenuItem(this->menu_id, "Settings...", MakeMenuRef(SettingsRef), 0);

#ifdef DEBUG_BUILD
    XPLMAppendMenuSeparator(this->menu_id);
    this->debug_id = XPLMAppendMenuItem(this->menu_id, "Debug", NULL, 0);
    this->debug_menu_id = XPLMCreateMenu("Debug", this->menu_id, this->debug_id, StaticMenuHandler, MakeMenuRef(DebugMenuRef));
    this->drawTestOSD_id = XPLMAppendMenuItem(this->debug_menu_id, "Draw Test OSD", MakeMenuRef(DrawTestOSDRef), 0);
    this->clearOSD_id = XPLMAppendMenuItem(this->debug_menu_id, "Clear OSD", MakeMenuRef(ClearOSDRef), 0);
    this->makeToast_id = XPLMAppendMenuItem(this->debug_menu_id, "Make Toast", MakeMenuRef(MakeToastRef), 0);
#endif
}

void Menu::MenuHandler(void *in_item_ref, void *in_menu_ref)
{
    auto plugin = Plugin();
    auto EventBus = plugin->GetEventBus();

    MenuItemRefs itemRef = RefToEnum<MenuItemRefs>(in_item_ref);
    MenuItemRefs menuRef = RefToEnum<MenuItemRefs>(in_menu_ref);

    switch (menuRef)
    {
    case MainMenuRef:
    {
        switch (itemRef)
        {
        case ConnectDisconnectHITLItemRef:
            EventBus->Publish("MenuConnectDisconnect", MenuConnectEventArg(false));
            break;
        case ConnectDisconnectSITLItemRef:
            EventBus->Publish("MenuConnectDisconnect", MenuConnectEventArg(true));
            break;
        case OsdVisibledRef:
        {    bool osdVisible = plugin->Settings()->GetSettingAs<bool>(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_OSD_VISIBLE, true);
            plugin->Settings()->SetSetting(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_OSD_VISIBLE, !osdVisible);
            break;
        }
        case MapDownloadWaypointsRef:
            EventBus->Publish("MenuMapDownloadWaypoints");
            break;
        case MapTeleportRef:
            EventBus->Publish("MenuMapTeleport");
            break;
        case SettingsRef:
            EventBus->Publish("MenuOpenSettings");
            break;
        case ShowGraphRef:
            EventBus->Publish("MenuOpenCloseGraph");
            break;
        case RebootINAVRef:
            EventBus->Publish("MenuRebootINAV");
            break;
        case KickStartAutolaunchRef:
            EventBus->Publish("MenuKickStartAutolaunch");
            break;
        default:
            break;
        }
        break;
    }

    case GpsMenuRef:
    {
        int sats = static_cast<int>(itemRef);
        plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_GPS_NUMSAT, sats);
        break;
    }
    case GpsFailureMenuRef:
    {
        switch (itemRef)
        {
        case GpsTimeoutRef:
        {
            bool current = plugin->Settings()->GetSettingAs<bool>(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_GPS_TIMEOUT, false);
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_GPS_TIMEOUT, !current);
            break;
        }
        case GpsGlitchFreezeRef:
        case GpsGlitchOffsetRef:
        case GpsGlitchLinearRef:
        case GpsGlitchAltitudeRef:
        {
            const int current = plugin->Settings()->GetSettingAs(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_GPS_GLITCH, 0);
            const int selected = static_cast<int>(itemRef) - static_cast<int>(GpsGlitchFreezeRef) + 1;
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_GPS_GLITCH, current == selected ? 0 : selected);
            break;
        }
        default:
            break;
        }
        break;
    }
    case CompassMenuRef:
    {
        switch (itemRef)
        {
        case MagNormalRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_MAG_FAILURE, false);
            break;
        case MagFailureRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_MAG_FAILURE, true);
            break;
        default:
            break;
        }
        break;
    }
    case OsdVisibilityMenuRef:
    {
        switch (itemRef)
        {
        case OsdVisibledRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_OSD_VISIBLE, true);
            break;
        case OsdDisabledRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_OSD_VISIBLE, false);
            break;
        default:
            break;
        }
        break;
    }

    case BatteryMenuRef:
    {
        switch (itemRef)
        {
        case BatteryNoneRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_BATTERY_EMULATION, 0);
            break;
        case BatteryInfiniteRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_BATTERY_EMULATION, 1);
            break;
        case Battery2200LipoRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_BATTERY_EMULATION, 2);
            break;
        case Battery4400LipoRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_BATTERY_EMULATION, 3);
            break;
        case Battery5200LionRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_BATTERY_EMULATION, 4);
            break;
        case Battery10400LionRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_BATTERY_EMULATION, 5);
            break;
        default:
            break;
        }
        break;
    }
    case PitotMenuRef:
    {
        switch (itemRef)
        {
        case PitotNoneRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_SIMULATE_PITOT, 0);
            break;
        case PitotSimulateRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_SIMULATE_PITOT, 1);
            break;
        case PitotFailureHWRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_SIMULATE_PITOT, 2);
            break;
        case PitotFailure60Ref:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_SIMULATE_PITOT, 3);
            break;
        default:
            break;
        }
        break;
    }
    case RangefinderMenuRef:
    {
        switch (itemRef)
        {
        case RangefinderNoneRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_SIMULATE_RANGEFINDER, 0);
            break;
        case RangefinderSimulateRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_SIMULATE_RANGEFINDER, 1);
            break;
        case RangefinderFailureRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_SIMULATE_RANGEFINDER, 2);
            break;
        default:
            break;
        }
        break;
    }
    case NoiseMenuRef:
    {
        switch (itemRef)
        {
        case NoiseNoneRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_VIDEOLINK_SIMULATION, 0);
            break;
        case Noise2KmRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_VIDEOLINK_SIMULATION, 1);
            break;
        case Noise10KmRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_VIDEOLINK_SIMULATION, 2);
            break;
        case Noise50KmRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_OSD, SettingsKeys::SETTINGS_VIDEOLINK_SIMULATION, 3);
            break;
        default:
            break;
        }
        break;
    }
    case RssiMenuRef:
    {
        switch (itemRef)
        {
        case RxToggleFailsafeRef:
            EventBus->Publish("MenuRssiToggleFailsafe");
            break;
        case RssiInfiniteRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_RSSI_SIMULATION, -1);
            break;
        case Rssi2KmRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_RSSI_SIMULATION, 2);
            break;
        case Rssi5KmRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_RSSI_SIMULATION, 5);
            break;
        case Rssi10KmRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_RSSI_SIMULATION, 10);
            break;
        case Rssi50KmRef:
            plugin->Settings()->SetSetting(SettingsSections::SECTION_SIMDATA, SettingsKeys::SETTINGS_RSSI_SIMULATION, 50);
            break;
        default:
            break;
        }
        break;
    }
#ifdef DEBUG_BUILD
    case DebugMenuRef:
    {
        switch (itemRef)
        {
        case DrawTestOSDRef:
            EventBus->Publish("MenuDebugDrawTestOSD");
            break;
        case ClearOSDRef:
            EventBus->Publish("MenuDebugClearOSD");
            break;
        case MakeToastRef:
            EventBus->Publish("MakeToast", OsdToastEventArg("Test Toast", "from Debug Menu", 10000));
            break;
        default:
            break;
        }
        break;
    }
#endif
    default:
        break;
    }

    plugin->Settings()->save();
}

void Menu::StaticMenuHandler(void *in_menu_ref, void *in_item_ref)
{
    Plugin()->Menu()->MenuHandler(in_item_ref, in_menu_ref);
}
