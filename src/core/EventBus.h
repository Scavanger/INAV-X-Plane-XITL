#pragma once

#include "../platform.h"

#include <string>
#include <functional>
#include <map>
#include <vector>
#include <typeinfo>
#include <memory>
#include <cstdint>
#include <any>
#include <stdexcept>

#include "../MathUtils.h"
#include "../MSP_Commands.h"
#include "../MSP.h"

using namespace MathUtils;

/**
 * @brief Event-based observer pattern for loose coupling between components
 */

class EventBus
{
private:
    // Void listeners
    std::map<std::string, std::vector<std::function<void()>>> voidListeners;
    
    // Typed listeners 
    struct TypedListenerEntry
    {
        std::type_info const* typeInfo;
        std::function<void(const std::any&)> callback;
    };
    std::map<std::string, std::vector<TypedListenerEntry>> typedListeners;

public:
    EventBus() = default;
    ~EventBus() = default;
    EventBus(const EventBus&) = delete;

    EventBus& operator=(const EventBus&) = delete;

    void Subscribe(const std::string& eventName, const std::function<void()>& listener)
    {
        voidListeners[eventName].push_back(listener);
    }
    
    template<typename EventType>
    void Subscribe(const std::string& eventName, const std::function<void(const EventType&)>& listener)
    {
        TypedListenerEntry entry;
        entry.typeInfo = &typeid(EventType);
        entry.callback = [listener](const std::any& event) {
            try {
                const auto& typedEvent = std::any_cast<const EventType&>(event);
                listener(typedEvent);
            } catch (const std::bad_any_cast&) {
                // Type mismatch, ignore
            }
        };
        typedListeners[eventName].push_back(entry);
    }

    void Publish(const std::string& eventName)
    {
        auto it = voidListeners.find(eventName);
        if (it != voidListeners.end())
        {
            for (auto& listener : it->second)
            {
                listener();
            }
        }
    }

    template<typename EventType>
    void Publish(const std::string& eventName, const EventType& event)
    {
        auto it = typedListeners.find(eventName);
        if (it != typedListeners.end())
        {
            for (auto& entry : it->second)
            {
                if (entry.typeInfo != nullptr && *entry.typeInfo == typeid(EventType))
                {
                    entry.callback(std::any(event));
                }
            }
        }
    }

    void Clear()
    {
        voidListeners.clear();
        typedListeners.clear();
    }
};

class SimulatorConnectedEventArg
{
public:
    ConnectionStatus status;
    
    SimulatorConnectedEventArg() = default;
    SimulatorConnectedEventArg(ConnectionStatus stat) : status(stat) {}
};

class OsdToastEventArg
{
public:
    std::string messageLine1 = "";
    std::string messageLine2 = "";  
    int durationMs = 2000;

    OsdToastEventArg() = default;
    OsdToastEventArg(const std::string& msgLine1, const std::string& msgLine2, int duration) : messageLine1(msgLine1), messageLine2(msgLine2), durationMs(duration) {}
};

class MSPMessageEventArg
{
public:
    const MSPCommand command;
    const std::vector<uint8_t> messageBuffer;

    MSPMessageEventArg() = default;
    MSPMessageEventArg(MSPCommand cmd) : command(cmd), messageBuffer() {};
    MSPMessageEventArg(MSPCommand cmd, const std::vector<uint8_t>& buffer) : command(cmd), messageBuffer(buffer) {}
};


class FlightLoopEventArg
{
public:
    float elapsedTime = 0.0f;
    int cycle = 0;

    FlightLoopEventArg() = default;
    FlightLoopEventArg(float elapsed, int ctr) : elapsedTime(elapsed), cycle(ctr) {}
};


class FontEventArg
{
    public:
    std::string fontName;
    std::string type;

    FontEventArg() = default;
    FontEventArg(const std::string& name, std::string type) : fontName(name), type(type) {}
};

class SettingsChangedEventArg
{
public:
    std::string sectionName;
    std::string settingName;
    std::string value;
    
    SettingsChangedEventArg() = default;
    SettingsChangedEventArg(const std::string& section, const std::string& setting, const std::string& val)
        : sectionName(section), settingName(setting), value(val) {}

    template<typename T>
    T getValueAs(const T stdValue) const
    {
        try {
            if constexpr (std::is_same_v<T, std::string>) {
                return value;
            } else if constexpr (std::is_integral_v<T>) {
                return static_cast<T>(std::stoll(value));
            } else if constexpr (std::is_floating_point_v<T>) {
                return static_cast<T>(std::stod(value));
            } else if constexpr (std::is_same_v<T, bool>) {
                return (value == "1" || value == "true" || value == "True");
            } else if constexpr (std::is_enum_v<T>) {
                return static_cast<T>(std::stoi(value));
            } else {
                return stdValue;
            }
        } catch (...) {
            return stdValue;
        } 
    }
};

class MenuConnectEventArg
{
public:
    bool toSitl = false;
    std::string sitlIp = "";
    int sitlPort = 0;
    
    MenuConnectEventArg() = default;
    MenuConnectEventArg(bool toSitl = false) : toSitl(toSitl), sitlIp(""), sitlPort(0) {}
    MenuConnectEventArg(bool toSitl, std::string ip, int port) : toSitl(toSitl), sitlIp(ip), sitlPort(port) {}
};

class DrawCallbackEventArg
{
public:
    int phase = 0;
    int isBefore = 0;
    
    DrawCallbackEventArg() = default;
    DrawCallbackEventArg(int phase, int isBefore) : phase(phase), isBefore(isBefore) {}
};

class Vector3EventArgs
{   
public:
    vector3D vector;

    Vector3EventArgs() = default;
    Vector3EventArgs(vector3D vec) : vector(vec) {}
    Vector3EventArgs(float x, float y, float z) : vector{ x, y, z } {}
};

class EulerAnglesEventArgs
{   
public:
    eulerAngles angles;

    EulerAnglesEventArgs() = default;
    EulerAnglesEventArgs(eulerAngles ang) : angles(ang) {}
    EulerAnglesEventArgs(float yaw, float pitch, float roll) : angles{ yaw, pitch, roll } {}
};

class Double3DPointEventArg
{
public:
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;

    Double3DPointEventArg() = default;
    Double3DPointEventArg(double latitude, double longitude, double altitude) : latitude(latitude), longitude(longitude), altitude(altitude) {}
}; 

class GraphTypeChangedEventArg
{
public:
    int graphType = 0;

    GraphTypeChangedEventArg() = default;
    GraphTypeChangedEventArg(int type) : graphType(type) {}
};

class FloatEventArg
{
public:
    float value = 0.0f;

    FloatEventArg() = default;
    FloatEventArg(float val) : value(val) {}
};

class IntEventArg
{   
public:
    int value = 0;

    IntEventArg() = default;
    IntEventArg(int val) : value(val) {}
};

class AddDebugEventArg
{
public:
    int index = 0;
    float value = 0.0f;

    AddDebugEventArg() = default;
    AddDebugEventArg(int idx, float val) : index(idx), value(val) {}
};

class UpdateDataRefEventArg
{
public: 
    int gpsNumSats = 0;
    int gpsFix = 0;
    float gpsLatitude = 0.0f;
    float gpsLongitude = 0.0f;
    float gpsElevation = 0.0f;
    float groundspeed = 0.0f;
    vector3D gpsVelocities;
    vector3D magnetometer;
    int rangefinderDistanceCm = 0;
    float airspeed = 0.0f;
    float batteryVoltage = 0.0f;
    float currentConsumption = 0.0f;
    float scaledThrottle = 0.0f;
    int rssi = 0;
    bool isFailsafe = false;

    UpdateDataRefEventArg() = default;
    UpdateDataRefEventArg(int sats, int fix, float lat, float lon, float ele, const vector3D& vel, const vector3D& mag,
                          int rangeCm, float airsp, float groundspeed, float volt, float curr, float throttle, int rssiVal, bool failsafe)
        : gpsNumSats(sats), gpsFix(fix), gpsLatitude(lat), gpsLongitude(lon), gpsElevation(ele), groundspeed(groundspeed), gpsVelocities(vel), magnetometer(mag),
          rangefinderDistanceCm(rangeCm), airspeed(airsp),  batteryVoltage(volt), currentConsumption(curr),
          scaledThrottle(throttle), rssi(rssiVal), isFailsafe(failsafe)
    {}
};
