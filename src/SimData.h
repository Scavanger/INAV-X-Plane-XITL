#pragma once

#include "platform.h"

#include <array>

#include "PowerTrain.h"
#include "MSP.h"
#include "Utils.h"
#include "MathUtils.h"

using namespace MathUtils;

namespace SimDataConstants
{
    // RSSI Emulation
    static constexpr int RSSI_MAX_VALUE = 1023;    // Maximum RSSI value (full signal)
    static constexpr int RSSI_MIN_VALUE = 0;       // Minimum RSSI value (no signal)
    static constexpr int RSSI_FAILSAFE_VALUE = 300; // RSSI value to trigger failsafe
    static constexpr float RSSI_INFINITE_RANGE = -1.0f; // Infinite range indicator
    // GPS Fix types
    static constexpr int GPS_NO_FIX = 0;
    static constexpr int GPS_FIX_2D = 1;
    static constexpr int GPS_FIX_3D = 2;

    // GPS Glitch modes
    static constexpr int GPS_GLITCH_NONE = 0;
    static constexpr int GPS_GLITCH_FREEZE = 1;
    static constexpr int GPS_GLITCH_OFFSET = 2;
    static constexpr int GPS_GLITCH_LINEAR = 3;
    static constexpr int GPS_GLITCH_ALTITUDE = 4;
    // RC Input
    static constexpr int RC_INPUT_CHANNELS = 8;

    // RC Channel indices
    static constexpr int RC_CHANNEL_ROLL = 0;
    static constexpr int RC_CHANNEL_PITCH = 1;
    static constexpr int RC_CHANNEL_THROTTLE = 2;
    static constexpr int RC_CHANNEL_YAW = 3;
    static constexpr int RC_CHANNEL_AUX1 = 4;
    static constexpr int RC_CHANNEL_AUX2 = 5;
    static constexpr int RC_CHANNEL_AUX3 = 6;
    static constexpr int RC_CHANNEL_AUX4 = 7;
    // Debug
    static constexpr int DEBUG_U32_COUNT = 8;
}

typedef enum
{
    BATTERY_NONE = 0,
    BATTERY_3S_LION_INFINITE = 1,
    BATTERY_3S_LIPO_2200MAH = 2,
    BATTERY_3S_LIPO_4400MAH = 3,
    BATTERY_3S_LION_5200MAH = 4,
    BATTERY_3S_LION_10400MAH = 5,
} BatteryEmulationType;

typedef enum
{
    None,
    Simulate,
    Failure,
    Failure60
} TPitotSimulation;

typedef enum
{
    RANGEFINDER_NONE = 0,
    RANGEFINDER_SIMULATE = 1,
    RANGEFINDER_FAILURE = 2
} TRangefinderSimulation;

// In Xplane format: floating point data
struct TSimdata
{
    int numSats;
    int fixType;
    float airspeed;
    float latitude;
    float longitude;
    float elevation;
    float speed;
    float course;
    vector3D acceleration;
    vector3D gyro;
    eulerAngles euler;
    vector3D velNED;
    vector3D mag;
    float baro;
    uint16_t rangefinder_distance_cm;
    float battery_voltage;
    float current_consumption;
};

#pragma pack(1)

struct TMSPSimultatorToINAVHeader
{
    uint8_t version; // MSP_SIMULATOR_VERSION
    uint16_t flags; // TSimulatorFlags
};

struct TMSPSimulatorToINAV
{
    TMSPSimultatorToINAVHeader header;

    uint8_t fix;
    uint8_t numSat;
    int32_t lat;
    int32_t lon;
    int32_t alt;
    int16_t speed;
    int16_t course;
    int16_t velNED[3];

    int16_t roll;
    int16_t pitch;
    int16_t yaw;

    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;

    int32_t baro;

    int16_t mag_x;
    int16_t mag_y;
    int16_t mag_z;

    // SIMU_EXT_BATTERY_VOLTAGE in format 2
    uint8_t vbat;      // 126->12.6V
    uint16_t airspeed; // cm/s

    uint16_t rangefinder_distance_cm; // cm

    uint16_t current;

    // RC inputs
    uint16_t rc_inputs[SimDataConstants::RC_INPUT_CHANNELS];
    uint16_t rssi;
};
#pragma pack()

class SimData
{
public:
    SimData();

private:
    //---- gps ---

    bool GPSHasNewData;
    uint32_t gps_lastUpdate;

    int gps_fix;
    int gps_glitch;
    bool gps_timeout;

    bool simulate_mag_failure;

    // degrees, the latitude of the aircraft
    XPLMDataRef df_latitude;

    // degrees, the longitude of the aircraft
    XPLMDataRef df_longitude;

    // meters, the elevation above MSL of the aircraft
    XPLMDataRef df_elevation;
    XPLMDataRef df_agl;

    // meters/s, The velocity in local OGL coordinates
    XPLMDataRef df_local_vx;

    XPLMDataRef df_local_vy;

    XPLMDataRef df_local_vz;

    // meters, altitude Above Ground Level
    // XPLMDataRef df_agl;
    // float agl;

    // meters/sec, the ground speed of the aircraft
    XPLMDataRef df_speed;

    // degrees, the true heading of the aircraft in degrees from the Z axis - OpenGL coordinates
    XPLMDataRef df_hpath;

    // degrees, the roll of the aircraft in degrees - OpenGL coordinates
    XPLMDataRef df_roll;
    // degrees, the pitch relative to the plane normal to the Y axis in degrees - OpenGL coordinates
    XPLMDataRef df_pitch;

    // degrees, the true heading of the aircraft in degrees from the Z axis - OpenGL coordinates
    XPLMDataRef df_yaw;

    // accel
    // Gs, total g-forces on the plane as a multiple, along the plane, forward - negative
    XPLMDataRef df_accel_x;

    // Gs, total g-forces on the plane as a multiple, sideways, right - negative
    XPLMDataRef df_accel_y;

    // Gs, Total g-forces on the plane as a multiple, downward
    XPLMDataRef df_accel_z;

    // gyro
    // degrees, the roll of the aircraft in degrees - OpenGL coordinates
    XPLMDataRef df_gyro_x;
    // deg/sec, the pitch rotation rates (relative to the flight)
    XPLMDataRef df_gyro_y;
    float gyro_y;

    // deg/sec	The yaw rotation rates (relative to the flight)
    XPLMDataRef df_gyro_z;

    // baro
    // 29.92+-....	This is the barometric pressure at the point the current flight is at.
    XPLMDataRef df_baro;

    // RC Inputs
    XPLMDataRef df_rc_inputs;
    float rc_inputs[SimDataConstants::RC_INPUT_CHANNELS];

    TPitotSimulation simulatePitot;

    // meters/sec, the ground speed of the aircraft
    XPLMDataRef df_airspeed;

    double glitch_lattitude = 0;
    double glitch_longitude = 0;
    double glitch_elevation = 0;

    //---- from inav --------

    XPLMDataRef df_override_joystick;

    XPLMDataRef df_control_throttle;
    int16_t control_throttle;
    XPLMDataRef df_control_roll;
    int16_t control_roll;
    XPLMDataRef df_control_pitch;
    int16_t control_pitch;
    XPLMDataRef df_control_yaw;
    int16_t control_yaw;

    XPLMDataRef df_heartbeat;

    bool isAirplane;
    bool isArmed;
    bool isOSDDisabled;
    bool isSupportedOSDNotFound;

    //-- state --
    bool muteBeeper;
    bool attitude_use_sensors;
    bool isHitlConnected;
    // SITL Dref Connection state
    bool isSitlConnected;

    // SITL MSP TCP connection state
    bool isSitlTcpConnected;

    uint32_t lastUpdateMS;
    uint32_t lastUpdateTimeFlightLoop;
    uint32_t sitlHartbeatLastTime;

    // Powertrain state

    BatteryEmulationType batEmulation;
    double powerTrainLastUpdate = 0;
    std::unique_ptr<PowerTrain> powerTrain;

    TRangefinderSimulation rangefinderSimulation;

    // Home Location for RSSI emulation
    double homeLocation_latitude = 0.0;
    double homeLocation_longitude = 0.0;
    double homeLocation_elevation = 0.0;
    bool homeLocation_isSet = false;
    double rxRangeKm = SimDataConstants::RSSI_INFINITE_RANGE;
    bool rxIsFailsafe = false;
    bool rxIsFailsafeFromMenu = false;

    uint32_t autolaunch_kickStart = 0;
    float agl;

    TSimdata simDataFromXplane;
    TSimdata simDataOut;

    void updateFromXPlane();
    void sendToXPlane_HITL();
    void sendToXPlane_SITL();

    void sendToINAV_SITL();
    void sendToINAV_HITL();

    void updateFromINAV(const TMSPSimulatorFromINAV &data);

    float getControllThrottle() const;
    uint16_t calculateRSSI();

    void updateDataRefs();
    void applyHardwareFailures(TSimdata& simData);

    void disconnect();

    void setBateryEmulation(BatteryEmulationType type);
    void recalculatePowerTrain();
};
