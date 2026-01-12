#pragma once

#include "platform.h"

#include <XPLMDataAccess.h>

#include "MathUtils.h"

using namespace MathUtils;

namespace DataRefsConstants {
    static constexpr int DEBUG_U32_COUNT = 8;
    static constexpr int XITL_DATAREF_VERSION = 2;
}


class DataRefs
{
public:
    DataRefs();
    ~DataRefs();

private:
    XPLMDataRef df_serialPacketsSent;
    int serialPacketsSent = 0;

    XPLMDataRef df_serialPacketsSentPerSecond;
    int serialPacketsSentPerSecond = 0;
    int serialPacketsSentLast = 0;

    XPLMDataRef df_serialBytesSent;
    int serialBytesSent = 0;

    XPLMDataRef df_serialBytesSentPerSecond;
    int serialBytesSentPerSecond = 0;
    int serialBytesSentLast = 0;

    XPLMDataRef df_serialPacketsReceived;
    int serialPacketsReceived = 0;

    XPLMDataRef df_serialPacketsReceivedPerSecond;
    int serialPacketsReceivedPerSecond = 0;
    int serialPacketsReceivedLast = 0;

    XPLMDataRef df_serialBytesReceived;
    int serialBytesReceived = 0;

    XPLMDataRef df_serialBytesReceivedPerSecond;
    int serialBytesReceivedPerSecond = 0;
    int serialBytesReceivedLast = 0;

    XPLMDataRef df_OSDUpdatesPerSecond;
    int OSDUpdates = 0;
    int OSDUpdatesLast = 0;
    int OSDUpdatesPerSecond = 0;

    XPLMDataRef df_eulerAngles;
    float dbg_eulerAngles[3] = {0, 0, 0};

    XPLMDataRef df_acc;
    float dbg_acc[3] = {0, 0, 0};

    XPLMDataRef df_gyro;
    float dbg_gyro[3] = {0, 0, 0};
    XPLMDataRef df_debug;
    int debug[DataRefsConstants::DEBUG_U32_COUNT] = {0};

    XPLMDataRef df_cyclesPerSecond;
    int cyclesPerSecond = 0;
    int cycles = 0;
    int cyclesLast = 0;

    XPLMDataRef df_XitlVersion;
    int xitlVersion = DataRefsConstants::XITL_DATAREF_VERSION; 

    uint32_t lastUpdate = 0;

    // SITL datarefs
    XPLMDataRef df_sitl_heartbeat;
    int sitl_heartbeat = 0;

    XPLMDataRef df_gps_numSats;
    int gps_numSats = 12;
    XPLMDataRef df_gps_fix;
    int gps_fix = 3;
    XPLMDataRef df_gps_latitude;
    float gps_latitude = 0.0;
    XPLMDataRef df_gps_longitude;
    float gps_longitude = 0.0;
    XPLMDataRef df_gps_elevation;
    float gps_elevation = 0.0;
    XPLMDataRef df_groundspeed;
    float groundspeed = 0.0f;
    XPLMDataRef df_gps_velocitys;
    float gps_velocitys[3] = {0, 0, 0};

    XPLMDataRef df_magnetometer;
    float magnetometer[3] = {0, 0, 0};

    XPLMDataRef df_rangefinder;
    int rangefinder_distance_cm = 0.0f;

    XPLMDataRef df_airspeed;
    float airspeed = 0.0f;

    XPLMDataRef df_current;
    float current = 0.0f;

    XPLMDataRef df_voltage;
    float voltage = 0.0f;

    // RC
    XPLMDataRef df_control_throttle;
    float control_throttle = 0.0f;

    XPLMDataRef df_rssi;
    int rssi = 0;

    XPLMDataRef df_failsafe;
    int isFailsafe = 0;

    void loop();

    XPLMDataRef registerIntDataRef(const char *pName, int *pValue, bool pIsReadOnly = true);
    XPLMDataRef registerFloatDataRef(const char *pName, float *pValue, bool pIsReadOnly = true);
    XPLMDataRef registerVector3DataRef(const char *pName, float *pValue);
};
