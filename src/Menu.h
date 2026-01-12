#pragma once

#include "platform.h"

#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <memory>
#include <XPLMMenus.h>

class Menu
{
private:
    std::map<std::string, int> analogFontMenuItems;
    std::map<std::string, int> digitalFontMenuItems;
    
    // Store font names so pointers remain valid for X-Plane menu system
    std::vector<std::shared_ptr<std::string>> fontNamesByMenuId;

    int menu_container_idx; // The index of our menu item in the Plugins menu

    XPLMMenuID menu_id;
    int connect_disconnect_hitl_id;
    int connect_disconnect_sitl_id;
    int osd_visibility_id;
    int map_download_waypoints;
    int map_teleport;
    int show_graph_id;
    int reboot_inav_id;
    int kickstart_autolaunch_id;

    XPLMMenuID hitlHardware_menu_id;
    int hitlHardware_id;

#ifdef DEBUG_BUILD
    XPLMMenuID debug_menu_id;
    int debug_id;
    int drawTestOSD_id;
    int clearOSD_id;
    int makeToast_id;
#endif


    XPLMMenuID gps_fix_menu_id;
    int gps_fix_id;
    int gps_fix_0_id;
    int gps_fix_3_id;
    int gps_fix_5_id;
    int gps_fix_12_id;
    
    
    XPLMMenuID gps_failure_menu_id;
    int gps_failure_id;
    int gps_timeout_id;
    int gps_freeze_id;
    int gps_offset_id;
    int gps_linear_id;
    int gps_altitude_id;

    XPLMMenuID battery_menu_id;
    int battery_id;
    int battery_none_id;
    int battery_infinite_id;
    int battery_3s_2200_id;
    int battery_3s_4400_id;
    int battery_3s_5200_id;
    int battery_3s_10400_id;

    XPLMMenuID mag_menu_id;
    int mag_id;
    int mag_normal_id;
    int mag_failure_id;

    XPLMMenuID pitot_menu_id;
    int pitot_id;
    int pitot_none_id;
    int pitot_simulate_id;
    int pitot_failure_hw_id;
    int pitot_failure_60_id;

    XPLMMenuID rangefinder_menu_id;
    int rangefinder_id;
    int rangefinder_none_id;
    int rangefinder_simulate_id;
    int rangefinder_failure_id;

    XPLMMenuID noise_menu_id;
    int noise_id;
    int noise_none_id;
    int noise_2KM_id;
    int noise_10KM_id;
    int noise_50KM_id;

    XPLMMenuID rssi_menu_id;
    int rssi_id;
    int trigger_failsafe_id;
    int rssi_inifinite_id;
    int rssi_2km_id;
    int rssi_5km_id;
    int rssi_10km_id;
    int rssi_50km_id;

    void CreateMenu();
    void MenuHandler(void *in_item_ref, void *in_menu_ref);

    static void StaticMenuHandler(void *in_menu_ref, void *in_item_ref);

public:
    Menu();
    ~Menu();
};
