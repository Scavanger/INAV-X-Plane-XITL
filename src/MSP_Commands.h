#pragma once

#include <cstdint>

enum MSPCommand : uint16_t
{
    MSP_API_VERSION = 0x01,
    MSP_FC_VARIANT  = 0x02,
    MSP_FC_VERSION  = 0x03,
    MSP_REBOOT      = 0x44,
    MSP_DISPLAYPORT = 0xB6,
    MSP_SIMULATOR   = 0x201F,
    MSP2_INAV_OSD_PREFERENCES = 0x2016,
    MSP_DEBUGMSG    = 0xFD,
    MSP_WP_GETINFO  = 0x14,
    MSP_WP          = 0x76    //out message         get a WP, WP# is in the payload, returns (WP#, lat, lon, alt, flags) WP#0-home, WP#16-poshold
};
