#include "DataRefs.h"

#include <XPLMPlugin.h>
#include <cstdio>

#include "core/PluginContext.h"
#include "core/EventBus.h"

#include "Utils.h"

namespace DataRefsConstants {
    static constexpr uint32_t MSG_ADD_DATAREF = 0x01000000;  //  Add dataref to DRE message
}

template <typename T>
static T readSingleValue(void *inRefcon)
{
    return *reinterpret_cast<T *>(inRefcon);
}

template <typename T>
static void writeSingleValue(void *inRefcon, T inValue)
{
    *reinterpret_cast<T *>(inRefcon) = inValue;
}

static int vector3ReadDataRef(void *inRefcon, float *outValues, int inOffset, int inMax)
{
    if (inRefcon != nullptr && outValues == nullptr)
    {
        return 3;
    }
    
    if (inRefcon == nullptr || inMax <= 0 || inOffset < 0 || inMax > 3 || inOffset >= 3)
    {
        return 0;
    }

    float *vec = reinterpret_cast<float *>(inRefcon);
    if (inOffset + inMax > 3)
    {
        inMax = 3 - inOffset;
    }
    for (int i = 0; i < inMax; i++)
    {
        outValues[i] = vec[inOffset + i];
    }
    return inMax;
};

XPLMDataRef DataRefs::registerIntDataRef(const char *pName, int *pValue, bool pIsReadOnly)
{
    XPLMDataRef res = XPLMRegisterDataAccessor
    (   pName,
        xplmType_Int, // The types we support
        pIsReadOnly ? 0 : 1,   // Writable ?
        readSingleValue<int>, pIsReadOnly ? NULL : writeSingleValue<int>,
        // Integer accessors
        NULL, NULL, // Float accessors
        NULL, NULL, // Doubles accessors
        NULL, NULL, // Int array accessors
        NULL, NULL, // Float array accessors
        NULL, NULL, // Raw data accessors
        pValue, pIsReadOnly ? NULL : pValue
    );

    XPLMPluginID PluginID = XPLMFindPluginBySignature("xplanesdk.examples.DataRefEditor");
    if (PluginID != XPLM_NO_PLUGIN_ID)
    {
        XPLMSendMessageToPlugin(PluginID, DataRefsConstants::MSG_ADD_DATAREF, (void *)pName);
    }
    return res;
}

XPLMDataRef DataRefs::registerFloatDataRef(const char *pName, float *pValue, bool pIsReadOnly)
{
    XPLMDataRef res = XPLMRegisterDataAccessor
    (
        pName,
        xplmType_Float, // The types we support
        pIsReadOnly ? 0 : 1,  // Writable?
        NULL, NULL,     // Integer accessors
        readSingleValue<float>, pIsReadOnly ? NULL : writeSingleValue<float>,
        // Float accessors
        NULL, NULL, // Doubles accessors
        NULL, NULL, // Int array accessors
        NULL, NULL, // Float array accessors
        NULL, NULL, // Raw data accessors
        pValue, pIsReadOnly ? NULL : pValue
    );

    XPLMPluginID PluginID = XPLMFindPluginBySignature("xplanesdk.examples.DataRefEditor");
    if (PluginID != XPLM_NO_PLUGIN_ID)
    {
        XPLMSendMessageToPlugin(PluginID, DataRefsConstants::MSG_ADD_DATAREF, (void *)pName);
    }
    return res;
}

XPLMDataRef DataRefs::registerVector3DataRef(const char *pName, float *pValue)
{
    XPLMDataRef res = XPLMRegisterDataAccessor
    (
        pName,
        xplmType_FloatArray, // The types we support
        0,            // Writable
        NULL, NULL,     // Integer accessors
        NULL, NULL,     // Float accessors
        NULL, NULL, // Doubles accessors
        NULL, NULL, // Int array accessors
        vector3ReadDataRef, NULL, // Float array accessors
        NULL, NULL, // Raw data accessors
        pValue, NULL
    );

    XPLMPluginID PluginID = XPLMFindPluginBySignature("xplanesdk.examples.DataRefEditor");
    if (PluginID != XPLM_NO_PLUGIN_ID)
    {
        XPLMSendMessageToPlugin(PluginID, DataRefsConstants::MSG_ADD_DATAREF, (void *)pName);
    }
    return res;
}


DataRefs::DataRefs()
{
    this->lastUpdate = Utils::GetTicks();
    auto eventBus = Plugin()->GetEventBus();

    this->df_serialPacketsSent = this->registerIntDataRef("inav_xitl/serial/packetsSent", &this->serialPacketsSent);
    this->df_serialPacketsSentPerSecond = this->registerIntDataRef("inav_xitl/serial/packetsSentPerSecond", &this->serialPacketsSentPerSecond);
    this->df_serialBytesSent = this->registerIntDataRef("inav_xitl/serial/bytesSent", &this->serialBytesSent);
    this->df_serialBytesSentPerSecond = this->registerIntDataRef("inav_xitl/serial/bytesSentPerSecond", &this->serialBytesSentPerSecond);
    this->df_serialPacketsReceived = this->registerIntDataRef("inav_xitl/serial/packetsReceived", &this->serialPacketsReceived);
    this->df_serialPacketsReceivedPerSecond = this->registerIntDataRef("inav_xitl/serial/packetsReceivedPerSecond", &this->serialPacketsReceivedPerSecond);
    this->df_serialBytesReceived = this->registerIntDataRef("inav_xitl/serial/bytesReceived", &this->serialBytesReceived);
    this->df_serialBytesReceivedPerSecond = this->registerIntDataRef("inav_xitl/serial/bytesReceivedPerSecond", &this->serialBytesReceivedPerSecond);
    this->df_cyclesPerSecond = this->registerIntDataRef("inav_xitl/debug/cyclesPerSecond", &this->cyclesPerSecond);
    this->df_cyclesPerSecond = this->registerIntDataRef("inav_xitl/debug/OSDUpdatesPerSecond", &this->OSDUpdatesPerSecond);

    this->df_eulerAngles = this->registerVector3DataRef("inav_xitl/inav/attitude.euler", this->dbg_eulerAngles);
    this->df_acc = this->registerVector3DataRef("inav_xitl/inav/acc.accADCf", this->dbg_acc);
    this->df_gyro = this->registerVector3DataRef("inav_xitl/inav/gyro.gyroADCf", this->dbg_gyro);

    this->xitlVersion = DataRefsConstants::XITL_DATAREF_VERSION;

    // Datarefs for SITL, avoid setting same values twice (via DREF over UDP and MSP over TCP)
    this->df_XitlVersion = this->registerIntDataRef("inav_xitl/plugin/xitlDrefVersion", &this->xitlVersion);
    this->df_sitl_heartbeat = this->registerIntDataRef("inav_xitl/plugin/heartbeat", &this->sitl_heartbeat, false);

    this->df_gps_numSats = this->registerIntDataRef("inav_xitl/gps/numSats", &this->gps_numSats);
    this->df_gps_fix = this->registerIntDataRef("inav_xitl/gps/fix", &this->gps_fix);
    this->df_gps_latitude = this->registerFloatDataRef("inav_xitl/gps/latitude", &this->gps_latitude);
    this->df_gps_longitude = this->registerFloatDataRef("inav_xitl/gps/longitude", &this->gps_longitude);
    this->df_gps_elevation = this->registerFloatDataRef("inav_xitl/gps/elevation", &this->gps_elevation);
    this->df_groundspeed = this->registerFloatDataRef("inav_xitl/gps/groundspeed", &this->groundspeed);
    this->df_gps_velocitys = this->registerVector3DataRef("inav_xitl/gps/velocities", this->gps_velocitys);

    this->df_magnetometer = this->registerVector3DataRef("inav_xitl/sensors/magnetometer", this->magnetometer);
    this->df_rangefinder = this->registerIntDataRef("inav_xitl/sensors/rangefinder", &this->rangefinder_distance_cm);
    this->df_airspeed = this->registerFloatDataRef("inav_xitl/sensors/airspeed", &this->airspeed);
    this->df_current = this->registerFloatDataRef("inav_xitl/sensors/battery_current", &this->current);
    this->df_voltage = this->registerFloatDataRef("inav_xitl/sensors/battery_voltage", &this->voltage);

    this->df_rssi = this->registerIntDataRef("inav_xitl/rc/rssi", &this->rssi);
    this->df_failsafe = this->registerIntDataRef("inav_xitl/rc/failsafe", &this->isFailsafe);

    // Use custom dataref accessors for arrays to support length query
    auto readDebugDataRef = []( void *inRefcon, int *outValues, int inOffset, int inCount)
    {
        if (inRefcon != nullptr && outValues == nullptr)
        {
            return DataRefsConstants::DEBUG_U32_COUNT;
        }
        
        if (inRefcon == nullptr || inCount <= 0 || inOffset < 0 || inCount > DataRefsConstants::DEBUG_U32_COUNT || inOffset >= DataRefsConstants::DEBUG_U32_COUNT)
        {
            return 0;
        }

        int *debug = reinterpret_cast<int *>(inRefcon);
        if (inOffset + inCount > DataRefsConstants::DEBUG_U32_COUNT)
        {
            inCount = DataRefsConstants::DEBUG_U32_COUNT - inOffset;
        }
        for (int i = 0; i < inCount; i++)
        {
            outValues[i] = debug[inOffset + i];
        }
        return inCount;
    };

    this->df_debug = XPLMRegisterDataAccessor
    (
        "inav_xitl/debug/debug",
        xplmType_IntArray, // The types we support
        0,            // Writable
        NULL, NULL,     // Integer accessors
        NULL, NULL,     // Float accessors
        NULL, NULL, // Doubles accessors
        readDebugDataRef, NULL, // Int array accessors
        NULL, NULL, // Float array accessors
        NULL, NULL, // Raw data accessors
        this->debug, NULL
    );

    this->df_acc = registerVector3DataRef("inav_xitl/inav/acc.accADCf", this->dbg_acc);
    this->df_gyro = registerVector3DataRef("inav_xitl/inav/gyro.gyroADCf", this->dbg_gyro);

    eventBus->Subscribe<FlightLoopEventArg>("FlightLoop", [this](const FlightLoopEventArg &event)
    { 
        this->loop(); 
        this->cycles++; 
    });

    eventBus->Subscribe("OSDFrameUpdated", [this]()
    { 
        this->OSDUpdates++; 
    });

    eventBus->Subscribe<EulerAnglesEventArgs>("AddAttitudeYPR", [this](const EulerAnglesEventArgs &event)
    {
        this->dbg_eulerAngles[0] = event.angles.pitch;
        this->dbg_eulerAngles[1] = event.angles.yaw;
        this->dbg_eulerAngles[2] = event.angles.roll;
    });

    eventBus->Subscribe<Vector3EventArgs>("AddGyro", [this](const Vector3EventArgs &event)
    {
        this->dbg_gyro[0] = event.vector.x;
        this->dbg_gyro[1] = event.vector.y;
        this->dbg_gyro[2] = event.vector.z;
    });

    eventBus->Subscribe<Vector3EventArgs>("AddACC", [this](const Vector3EventArgs &event)
    {
        this->dbg_acc[0] = event.vector.x;
        this->dbg_acc[1] = event.vector.y;
        this->dbg_acc[2] = event.vector.z;
    });

    eventBus->Subscribe<AddDebugEventArg>("AddDebug", [this](const AddDebugEventArg &event)
    {
        if (event.index >= 0 && event.index < DataRefsConstants::DEBUG_U32_COUNT)
        {
            this->debug[event.index] = event.value;
        }
    });

    eventBus->Subscribe<IntEventArg>("SerialBytesReceived", [this](const IntEventArg &event)    
    {
        this->serialBytesReceived += event.value;
        this->serialPacketsReceived++; 
    });

    eventBus->Subscribe<IntEventArg>("SerialBytesSent", [this](const IntEventArg &event)
    {
        this->serialBytesSent += event.value;
        this->serialPacketsSent++; 
    });

    eventBus->Subscribe<UpdateDataRefEventArg>("UpdateDataRef", [this](const UpdateDataRefEventArg &event)
    {
        this->gps_numSats = event.gpsNumSats;
        this->gps_fix = event.gpsFix;
        this->gps_latitude = event.gpsLatitude;
        this->gps_longitude = event.gpsLongitude;
        this->gps_elevation = event.gpsElevation;
        this->groundspeed = event.groundspeed;
        this->gps_velocitys[0] = event.gpsVelocities.x;
        this->gps_velocitys[1] = event.gpsVelocities.y;
        this->gps_velocitys[2] = event.gpsVelocities.z;
        this->magnetometer[0] = event.magnetometer.x;
        this->magnetometer[1] = event.magnetometer.y;
        this->magnetometer[2] = event.magnetometer.z;
        this->rangefinder_distance_cm = event.rangefinderDistanceCm;
        this->airspeed = event.airspeed;
        this->current = event.currentConsumption;
        this->voltage = event.batteryVoltage;
        this->rssi = event.rssi;
        this->isFailsafe = event.isFailsafe ? 1 : 0;
    });
}

void DataRefs::loop()
{
    uint32_t delta = Utils::GetTicks() - this->lastUpdate;
    if (delta >= 1000)
    {
        this->serialBytesSentPerSecond = this->serialBytesSent - this->serialBytesSentLast;
        this->serialBytesSentLast = this->serialBytesSent;

        this->serialPacketsSentPerSecond = this->serialPacketsSent - this->serialPacketsSentLast;
        this->serialPacketsSentLast = this->serialPacketsSent;

        this->serialBytesReceivedPerSecond = this->serialBytesReceived - this->serialBytesReceivedLast;
        this->serialBytesReceivedLast = this->serialBytesReceived;

        this->serialPacketsReceivedPerSecond = this->serialPacketsReceived - this->serialPacketsReceivedLast;
        this->serialPacketsReceivedLast = this->serialPacketsReceived;

        this->cyclesPerSecond = this->cycles - this->cyclesLast;
        this->cyclesLast = this->cycles;

        this->OSDUpdatesPerSecond = this->OSDUpdates - this->OSDUpdatesLast;
        this->OSDUpdatesLast = this->OSDUpdates;

        this->lastUpdate = Utils::GetTicks();
    }
}

DataRefs::~DataRefs()
{
    XPLMUnregisterDataAccessor(this->df_serialPacketsSent);
    XPLMUnregisterDataAccessor(this->df_serialPacketsSentPerSecond);
    XPLMUnregisterDataAccessor(this->df_serialBytesSent);
    XPLMUnregisterDataAccessor(this->df_serialBytesSentPerSecond);
    XPLMUnregisterDataAccessor(this->df_serialPacketsReceived);
    XPLMUnregisterDataAccessor(this->df_serialPacketsReceivedPerSecond);
    XPLMUnregisterDataAccessor(this->df_serialBytesReceived);
    XPLMUnregisterDataAccessor(this->df_serialBytesReceivedPerSecond);
    XPLMUnregisterDataAccessor(this->df_OSDUpdatesPerSecond);
    
    XPLMUnregisterDataAccessor(this->df_eulerAngles);
    XPLMUnregisterDataAccessor(this->df_acc);
    XPLMUnregisterDataAccessor(this->df_gyro);

    XPLMUnregisterDataAccessor(this->df_debug);
    XPLMUnregisterDataAccessor(this->df_cyclesPerSecond);

    XPLMUnregisterDataAccessor(this->df_XitlVersion);
    XPLMUnregisterDataAccessor(this->df_sitl_heartbeat);

    XPLMUnregisterDataAccessor(this->df_gps_numSats);
    XPLMUnregisterDataAccessor(this->df_gps_fix);
    XPLMUnregisterDataAccessor(this->df_gps_latitude);
    XPLMUnregisterDataAccessor(this->df_gps_longitude);
    XPLMUnregisterDataAccessor(this->df_gps_elevation);
    XPLMUnregisterDataAccessor(this->df_groundspeed);
    XPLMUnregisterDataAccessor(this->df_gps_velocitys);
    XPLMUnregisterDataAccessor(this->df_magnetometer);
    XPLMUnregisterDataAccessor(this->df_rangefinder);
    XPLMUnregisterDataAccessor(this->df_airspeed);
    XPLMUnregisterDataAccessor(this->df_current);
    XPLMUnregisterDataAccessor(this->df_voltage);
    XPLMUnregisterDataAccessor(this->df_rssi);
    XPLMUnregisterDataAccessor(this->df_failsafe);

    XPLMUnregisterDataAccessor(this->df_control_throttle);
}