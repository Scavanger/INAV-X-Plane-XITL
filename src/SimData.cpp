#include <numbers>
#include <cstring>
#include <map>

#include "core/PluginContext.h"
#include "core/EventBus.h"

#include "settings/SettingNames.h"
#include "SimData.h"

#include <math.h>

struct BatteryData
{
    BatteryChemistryType chemistry;
    double voltage;
    int capacityMah;
};

namespace SimDataConstants
{
    static constexpr unsigned int MSP_PERIOD_MS = 10u;
    static constexpr int GPS_RATE_HZ = 5;
    static constexpr float GRAVITY_MSS = 9.80665f;

    static constexpr int SITL_HEARTBEAT_TIMEOUT = 500; // 0.5 seconds
    static constexpr float MAX_RANEGEFINDER_DISTANCE_CM = 1000.0f; // 10 meters

    static const std::map<BatteryEmulationType, BatteryData> BATTERY_DATA = {
        {BATTERY_NONE, {Lipo, 0.0, 0}},
        {BATTERY_3S_LION_INFINITE, {Lion, 12.6, 100000}}, // effectively infinite
        {BATTERY_3S_LIPO_2200MAH, {Lipo, 12.6, 2200}},
        {BATTERY_3S_LIPO_4400MAH, {Lipo, 12.6, 4400}},
        {BATTERY_3S_LION_5200MAH, {Lion, 12.6, 5200}},
        {BATTERY_3S_LION_10400MAH, {Lion, 12.6, 10400}},
    };

    // PWN rage is 1000 - 2000
    static inline uint16_t float_0_1_to_pwm(float x)
    {
        return static_cast<uint16_t>(x * 1000.0f) + 1000;
    }

    static inline uint16_t float_minus_1_1_to_pwm(float x)
    {
        return static_cast<uint16_t>((x + 1.0f) * 500.0f) + 1000;
    }

    // Input range is -500 to 500 for roll, pitch, yaw
    static inline float input_to_float_0_1(int16_t input)
    {
        return (static_cast<float>(input) + 500.0f) / 1000.0f;
    }

    static inline float input_to_float_minus_1_1(int16_t input)
    {
        return static_cast<float>(input) / 500.0;
    }

    static inline int16_t float_0_1_to_input(float x)
    {
        return static_cast<int16_t>(x * 1000.0f) - 500;
    }
}

SimData::SimData() : isHitlConnected(false)
{
    this->gps_lastUpdate = Utils::GetTicks() - 1000;
    this->sitlHartbeatLastTime = Utils::GetTicks() - 1000;
    this->GPSHasNewData = false;

    this->gps_fix = SimDataConstants::GPS_FIX_3D;
    simDataFromXplane.numSats = 12;
    this->gps_glitch = SimDataConstants::GPS_GLITCH_NONE;
    this->gps_timeout = false;

    this->simulate_mag_failure = false;

    this->df_latitude = XPLMFindDataRef("sim/flightmodel/position/latitude");
    this->df_longitude = XPLMFindDataRef("sim/flightmodel/position/longitude");
    this->df_elevation = XPLMFindDataRef("sim/flightmodel/position/elevation");
    this->df_agl = XPLMFindDataRef("sim/flightmodel/position/y_agl");

    this->df_local_vx = XPLMFindDataRef("sim/flightmodel/position/local_vx");
    this->df_local_vy = XPLMFindDataRef("sim/flightmodel/position/local_vy");
    this->df_local_vz = XPLMFindDataRef("sim/flightmodel/position/local_vz");

    this->df_speed = XPLMFindDataRef("sim/flightmodel/position/groundspeed");
    this->df_airspeed = XPLMFindDataRef("sim/flightmodel/position/true_airspeed");

    this->df_roll = XPLMFindDataRef("sim/flightmodel/position/phi");
    this->df_pitch = XPLMFindDataRef("sim/flightmodel/position/theta");
    this->df_yaw = XPLMFindDataRef("sim/flightmodel/position/psi");
    this->df_hpath = XPLMFindDataRef("sim/flightmodel/position/hpath");

    // Accelerometer
    this->df_accel_x = XPLMFindDataRef("sim/flightmodel/forces/g_axil");
    this->df_accel_y = XPLMFindDataRef("sim/flightmodel/forces/g_side");
    this->df_accel_z = XPLMFindDataRef("sim/flightmodel/forces/g_nrml");

    // Gyro
    this->df_gyro_x = XPLMFindDataRef("sim/flightmodel/position/P");
    this->df_gyro_y = XPLMFindDataRef("sim/flightmodel/position/Q");
    this->df_gyro_z = XPLMFindDataRef("sim/flightmodel/position/R");

    // Barometer
    this->df_baro = XPLMFindDataRef("sim/weather/barometer_current_inhg");

    // RC Inputs
    this->df_rc_inputs = XPLMFindDataRef("sim/joystick/joy_mapped_axis_value");

    // Our own Dataref to get INAVs heartbeat in SITL mode
    this->df_heartbeat = XPLMFindDataRef("inav_xitl/plugin/heartbeat");


    // // Velocity
    // xPlane.requestDataRef('sim/flightmodel/forces/vx_acf_axis', 10, function (ref, value) {
    //     simData.velocity_x = value; isSimDataUpdated = true;
    // });
    // xPlane.requestDataRef('sim/flightmodel/forces/vy_acf_axis', 10, function (ref, value) {
    //     simData.velocity_y = value; isSimDataUpdated = true;
    // });
    // xPlane.requestDataRef('sim/flightmodel/forces/vz_acf_axis', 10, function (ref, value) {
    //     simData.velocity_z = value; isSimDataUpdated = true;
    // });
    // xPlane.setDataRef('sim/operation/override/override_control_surfaces', 1);

    // static XPLMDataRef wrt = XPLMFindDataRef("sim/graphics/view/world_render_type");


    this->simulatePitot = TPitotSimulation::Simulate;
    this->simDataFromXplane.airspeed = 0;
    this->agl = 0;
    this->sitlHartbeatLastTime = 0;
    this->isSitlConnected = false;
    this->isSitlTcpConnected = false;

    //---- output ----
    this->muteBeeper = true;
    this->attitude_use_sensors = false;
    this->control_throttle = -500;

    this->lastUpdateMS = this->lastUpdateTimeFlightLoop = 0;

    this->df_hasJoystick = XPLMFindDataRef("sim/joystick/has_joystick");
    this->df_override_joystick = XPLMFindDataRef("sim/operation/override/override_joystick");
    this->df_control_throttle = XPLMFindDataRef("sim/cockpit2/engine/actuators/throttle_ratio_all");
    this->df_control_roll = XPLMFindDataRef("sim/joystick/yoke_roll_ratio");
    this->df_control_pitch = XPLMFindDataRef("sim/joystick/yoke_pitch_ratio");
    this->df_control_yaw = XPLMFindDataRef("sim/joystick/yoke_heading_ratio");

    this->isAirplane = false;
    this->isArmed = false;
    this->isOSDDisabled = false;
    this->isSupportedOSDNotFound = false;

    this->setBateryEmulation(BATTERY_3S_LION_INFINITE);

    auto eventBus = Plugin()->GetEventBus();

    eventBus->Subscribe<FlightLoopEventArg>(
        "FlightLoop",
        [this](const FlightLoopEventArg &event)
        {
            if ((Utils::GetTicks() - this->lastUpdateTimeFlightLoop) > SimDataConstants::MSP_PERIOD_MS)
            {
                this->updateFromXPlane();
                this->updateDataRefs();

                if (this->isHitlConnected)
                {
                    this->sendToINAV_HITL();
                } 
                else if (this->isSitlConnected)
                {
                    
                    this->sendToINAV_SITL();
                }
                this->lastUpdateMS = Utils::GetTicks();
            }
        });

    eventBus->Subscribe<SimulatorConnectedEventArg>(
        "SimulatorConnected",
        [this](const SimulatorConnectedEventArg &event)
        {
            if (event.status == ConnectionStatus::ConnectedHitl)
            {
                this->setBateryEmulation(this->batEmulation);
                this->isHitlConnected = true;
                this->rxIsFailsafe = false;
                this->rxIsFailsafeFromMenu = false;
            }
            else if (event.status == ConnectionStatus::ConnectedSitl)
            {
                if (!this->isSitlConnected)
                {
                    Plugin()->GetEventBus()->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("SITL not connected", "via DREF", 3000));
                    return;
                }
                
                this->setBateryEmulation(this->batEmulation);
                this->isSitlTcpConnected = true;
                this->rxIsFailsafe = false;
                this->rxIsFailsafeFromMenu = false;
            }
            else
            {
                this->isHitlConnected = false;
                this->isSitlTcpConnected = false;
                this->disconnect();
            }
        });

    eventBus->Subscribe<MSPMessageEventArg>(
        "MSPMessage",
        [this](const MSPMessageEventArg &event)
        {
            if (event.messageBuffer.size() < MSPConstants::MSP_SIMULATOR_RESPOSE_MIN_LENGTH)
            {
                Plugin()->GetEventBus()->Publish<SimulatorConnectedEventArg>("SimulatorConnected", SimulatorConnectedEventArg(ConnectionStatus::Disconnected));
                Plugin()->GetEventBus()->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("Disconnected", "Unsupported firmware", 3000));
                Utils::LOG("Unsupported firmware version, MSP_SIMULATOR response length: {}", event.messageBuffer.size());
            }
            else
            {
    
                TMSPSimulatorFromINAV msg;

                if (event.messageBuffer.size() > sizeof(msg))
                {
                    return;
                }

                std::memcpy(&msg, event.messageBuffer.data(), event.messageBuffer.size());

                this->updateFromINAV(msg);

                if (!this->isAirplane)
                {
                    Plugin()->GetEventBus()->Publish<SimulatorConnectedEventArg>("SimulatorConnected", SimulatorConnectedEventArg(ConnectionStatus::Disconnected));
                    Plugin()->GetEventBus()->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("Disconnected", "Unsupported aircraft type", 3000));
                    Utils::LOG("Unsupported aircraft type");
                }
                else
                {
                    if (this->isOSDDisabled)
                    {
                        Plugin()->GetEventBus()->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("OSD disabled", "Enable OSD in INAV", 3000));
                    }
                    else if (this->isSupportedOSDNotFound)
                    {
                        Plugin()->GetEventBus()->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("NO OSD", "Configure OSD in INAV", 3000));
                    }

                    if (this->isHitlConnected)
                    {
                        this->sendToXPlane_HITL();
                    } 
                    else if (this->isSitlConnected && this->isSitlTcpConnected)
                    {
                        this->sendToXPlane_SITL();
                    }
                }
            }
        });

    eventBus->Subscribe<Double3DPointEventArg>(
        "UpdateHomeLocation",
        [this](const Double3DPointEventArg &event)
        {
            // For RSSI emulation
            this->homeLocation_latitude = this->simDataFromXplane.latitude;
            this->homeLocation_longitude = this->simDataFromXplane.longitude;
            this->homeLocation_elevation = this->simDataFromXplane.elevation;
            this->homeLocation_isSet = true;
        });

    eventBus->Subscribe<SettingsChangedEventArg>(
        "SettingsChanged",
        [this](const SettingsChangedEventArg &event)
        {
            if (event.sectionName != SettingsSections::SECTION_SIMDATA)
            {
                return;
            }

            if (event.settingName == SettingsKeys::SETTINGS_GPS_NUMSAT)
            {
                this->simDataFromXplane.numSats = event.getValueAs<int>(12);
                if (this->simDataFromXplane.numSats == 0)
                {
                    this->gps_fix = SimDataConstants::GPS_NO_FIX;
                }
                else if (this->simDataFromXplane.numSats < 4)
                {
                    this->gps_fix = SimDataConstants::GPS_FIX_2D;
                }
                else
                {
                    this->gps_fix = SimDataConstants::GPS_FIX_3D;
                }
            }
            else if (event.settingName == SettingsKeys::SETTINGS_GPS_TIMEOUT)
            {
                this->gps_timeout = event.getValueAs<bool>(false);
            }
            else if (event.settingName == SettingsKeys::SETTINGS_GPS_GLITCH)
            {
                this->gps_glitch = event.getValueAs<int>(0);
            }
            else if (event.settingName == SettingsKeys::SETTINGS_MAG_FAILURE)
            {
                this->simulate_mag_failure = event.getValueAs<bool>(false);
            }
            else if (event.settingName == SettingsKeys::SETTINGS_ATTITUDE_COPY_FROM_XPLANE)
            {
                this->attitude_use_sensors = !event.getValueAs<bool>(false);
            }
            else if (event.settingName == SettingsKeys::SETTINGS_BATTERY_EMULATION)
            {
                auto batEmu = event.getValueAs<BatteryEmulationType>(BATTERY_3S_LION_INFINITE);
                this->setBateryEmulation(batEmu);
            }
            else if (event.settingName == SettingsKeys::SETTINGS_MUTE_BEEPER)
            {
                this->muteBeeper = event.getValueAs<bool>(true);
            }
            else if (event.settingName == SettingsKeys::SETTINGS_SIMULATE_PITOT)
            {
                this->simulatePitot = event.getValueAs<TPitotSimulation>(None);
            }
            else if (event.settingName == SettingsKeys::SETTINGS_SIMULATE_RANGEFINDER)
            {
                this->rangefinderSimulation = event.getValueAs<TRangefinderSimulation>(RANGEFINDER_NONE);
            }
            else if (event.settingName == SettingsKeys::SETTINGS_RSSI_SIMULATION)
            {
                this->rxRangeKm = event.getValueAs<float>(SimDataConstants::RSSI_INFINITE_RANGE);
            }
        });

    eventBus->Subscribe(
        "MenuRssiToggleFailsafe", 
        [this]()
        { 
            this->rxIsFailsafe = !this->rxIsFailsafe; 
            this->rxIsFailsafeFromMenu = !this->rxIsFailsafeFromMenu;
        }
    );

    eventBus->Subscribe(
        "MenuKickStartAutolaunch", 
        [this]()
        { 
            this->autolaunch_kickStart = Utils::GetTicks();
        }
    );
}

void SimData::updateFromXPlane()
{
    static bool firstUpdate = true;

    auto eventBus = Plugin()->GetEventBus();
    uint32_t t = Utils::GetTicks();

    // SITL sets heartbeat dataref to a positive value every sitl update cycle
    const int heartbeat = XPLMGetDatai(this->df_heartbeat);
    if (heartbeat >= 1)
    {
        this->isSitlConnected = true;
        this->sitlHartbeatLastTime = t;
        XPLMSetDatai(this->df_heartbeat, 0);
    }
    
    if (heartbeat == 0 && (t - this->sitlHartbeatLastTime) > SimDataConstants::SITL_HEARTBEAT_TIMEOUT)
    {
        this->isSitlConnected = false;
    }
    
    if ((t - this->gps_lastUpdate) >= (1000 / SimDataConstants::GPS_RATE_HZ))
    {
        this->gps_lastUpdate = t;
        this->GPSHasNewData = true;

        this->simDataFromXplane.latitude = XPLMGetDatad(this->df_latitude);
        this->simDataFromXplane.longitude = XPLMGetDatad(this->df_longitude);
        this->simDataFromXplane.elevation = XPLMGetDatad(this->df_elevation);
        this->agl = XPLMGetDataf(this->df_agl);

        this->simDataFromXplane.velNED.x = XPLMGetDataf(this->df_local_vx);
        this->simDataFromXplane.velNED.y = XPLMGetDataf(this->df_local_vy);
        this->simDataFromXplane.velNED.z = XPLMGetDataf(this->df_local_vz);
        this->simDataFromXplane.speed = XPLMGetDataf(this->df_speed);
        this->simDataFromXplane.airspeed = XPLMGetDataf(this->df_airspeed);
        this->simDataFromXplane.course = XPLMGetDataf(this->df_hpath);

        if (firstUpdate)
        {
            firstUpdate = false;
            eventBus->Publish<Double3DPointEventArg>("UpdateHomeLocation", Double3DPointEventArg(this->simDataFromXplane.latitude, this->simDataFromXplane.longitude, this->simDataFromXplane.elevation));
        }
    }

    if (agl > 0.0f && agl * 100 < SimDataConstants::MAX_RANEGEFINDER_DISTANCE_CM)
    {
        this->simDataFromXplane.rangefinder_distance_cm = roundf(this->agl * 100.0f);
    }
    else
    {
        this->simDataFromXplane.rangefinder_distance_cm = 0xffff; // out of range
    }

    this->simDataFromXplane.euler.roll = XPLMGetDataf(this->df_roll);
    this->simDataFromXplane.euler.pitch = XPLMGetDataf(this->df_pitch);
    this->simDataFromXplane.euler.yaw = XPLMGetDataf(this->df_yaw);

    eventBus->Publish<Double3DPointEventArg>("UpdatePosition", Double3DPointEventArg(this->simDataFromXplane.latitude, this->simDataFromXplane.longitude, this->simDataFromXplane.elevation));
    eventBus->Publish<FloatEventArg>("UpdateRoll", FloatEventArg(this->simDataFromXplane.euler.roll));
    float kick = 0;
    if (this->autolaunch_kickStart != 0)
    {
        uint32_t dt = t - this->autolaunch_kickStart;
        if (dt > 1000)
        {
            this->autolaunch_kickStart = 0;
        }
        else
        {
            kick = 4 * sin(dt / 180.0f * std::numbers::pi);
        }
    }
    this->simDataFromXplane.acceleration.x = XPLMGetDataf(this->df_accel_x) + kick;
    this->simDataFromXplane.acceleration.y = XPLMGetDataf(this->df_accel_y);
    this->simDataFromXplane.acceleration.z = XPLMGetDataf(this->df_accel_z);

    this->simDataFromXplane.gyro.x = XPLMGetDataf(this->df_gyro_x);
    this->simDataFromXplane.gyro.y = XPLMGetDataf(this->df_gyro_y);
    this->simDataFromXplane.gyro.z = XPLMGetDataf(this->df_gyro_z);
    this->simDataFromXplane.baro = XPLMGetDataf(this->df_baro);

    vector3D north = {1.0, 0.0, 0.0};
    quaternion quat = computeQuaternionFromEuler(this->simDataFromXplane.euler);
    this->simDataFromXplane.mag = transformVectorEarthToBody(north, quat);

    if (this->isHitlConnected)
    {
    
        this->hasJoystick = XPLMGetDatai(this->df_hasJoystick) != 0;      
        XPLMGetDatavf(this->df_rc_inputs, &this->rc_inputs[SimDataConstants::RC_CHANNEL_PITCH], 1, 1);
        XPLMGetDatavf(this->df_rc_inputs, &this->rc_inputs[SimDataConstants::RC_CHANNEL_ROLL], 2, 1);
        XPLMGetDatavf(this->df_rc_inputs, &this->rc_inputs[SimDataConstants::RC_CHANNEL_YAW], 3, 1);
        XPLMGetDatavf(this->df_rc_inputs, &this->rc_inputs[SimDataConstants::RC_CHANNEL_THROTTLE], 57, 1);
        XPLMGetDatavf(this->df_rc_inputs, &this->rc_inputs[SimDataConstants::RC_CHANNEL_AUX1], 58, 1);
        XPLMGetDatavf(this->df_rc_inputs, &this->rc_inputs[SimDataConstants::RC_CHANNEL_AUX2], 59, 1);
        XPLMGetDatavf(this->df_rc_inputs, &this->rc_inputs[SimDataConstants::RC_CHANNEL_AUX3], 60, 1);
        XPLMGetDatavf(this->df_rc_inputs, &this->rc_inputs[SimDataConstants::RC_CHANNEL_AUX4], 61, 1);
    }

    eventBus->Publish<EulerAnglesEventArgs>(
        "AddAttitudeYPR",
        EulerAnglesEventArgs(simDataFromXplane.euler)
    );

    eventBus->Publish<Vector3EventArgs>(
        "AddACC",
        Vector3EventArgs(
            -simDataFromXplane.acceleration.x,
            simDataFromXplane.acceleration.y,
            simDataFromXplane.acceleration.z)
    );

    eventBus->Publish<Vector3EventArgs>(
        "AddGyro",
        Vector3EventArgs(
            simDataFromXplane.gyro.x,
            -simDataFromXplane.gyro.y,
            -simDataFromXplane.gyro.z)
    );
}

float SimData::getControllThrottle() const
{
    float throttle = SimDataConstants::input_to_float_0_1(this->control_throttle);
    if (this->batEmulation != BATTERY_NONE)
    {
        throttle = this->powerTrain->getMotorThrottleFactor();
    }
    return std::clamp(throttle, 0.0f, 1.0f);
}

void SimData::sendToXPlane_HITL()
{
    XPLMSetDatai(this->df_override_joystick, 1);
    XPLMSetDataf(this->df_control_throttle, getControllThrottle());
    XPLMSetDataf(this->df_control_roll, SimDataConstants::input_to_float_minus_1_1(this->control_roll));
    XPLMSetDataf(this->df_control_pitch, -SimDataConstants::input_to_float_minus_1_1(this->control_pitch));
    XPLMSetDataf(this->df_control_yaw, -SimDataConstants::input_to_float_minus_1_1(this->control_yaw));
}

void SimData::sendToXPlane_SITL()
{
    // In SITL mode we just send the throttle command, other controls are handled by INAV itself
    XPLMSetDatai(this->df_override_joystick, 1);
    XPLMSetDataf(this->df_control_throttle, getControllThrottle());
}

void SimData::updateFromINAV(const TMSPSimulatorFromINAV &data)
{
    auto eventBus = Plugin()->GetEventBus();

    int dgbIdx = data.debugIndex & 0xf;

    this->control_throttle = data.throttle;
    this->control_roll = data.roll;
    this->control_pitch = data.pitch;
    this->control_yaw = data.yaw;

    this->isAirplane = (data.debugIndex & FIF_IS_AIRPLANE) != 0;
    bool prevArmed = this->isArmed;
    this->isArmed = (data.debugIndex & FIF_ARMED) != 0;
    this->isOSDDisabled = (data.debugIndex & FIF_OSD_DISABLED) != 0;
    this->isSupportedOSDNotFound = (data.debugIndex & FIF_ANALOG_OSD_NOT_FOUND) != 0;

    if (this->isArmed && !prevArmed)
    {
        Utils::DisableBrakes();
        eventBus->Publish<Double3DPointEventArg>("UpdateHomeLocation", Double3DPointEventArg(this->simDataFromXplane.latitude, this->simDataFromXplane.longitude, this->simDataFromXplane.elevation));
    }

    eventBus->Publish<Vector3EventArgs>(
        "AddEstimatedAttitudeYPR",
        Vector3EventArgs(
            data.estimated_attitude_roll,
            data.estimated_attitude_pitch,
            data.estimated_attitude_yaw)
    );

    eventBus->Publish<Vector3EventArgs>(
        "AddOutputYPR",
        Vector3EventArgs(
            this->control_yaw,
            this->control_pitch,
            this->control_roll)
    );

    eventBus->Publish<AddDebugEventArg>(
        "AddDebug",
        AddDebugEventArg(
            data.debugIndex & 7,
            data.debugValue)
    );

    uint32_t t = Utils::GetTicks();
    uint32_t delta = t - this->lastUpdateMS;
    if ((this->lastUpdateMS != 0) && (delta < 300))
    {
        eventBus->Publish<IntEventArg>(
            "AddUpdatePeriodMS",
            IntEventArg(delta)
        );
    }
    this->lastUpdateMS = t;
}


void SimData::sendToINAV_SITL()
{
    if (!this->isSitlTcpConnected)
    {
        return;
    }
    
    this->recalculatePowerTrain();

    TMSPSimultatorToINAVHeader header = {0};
    header.version = MSPConstants::MSP_SIMULATOR_VERSION;
    header.flags = SIMU3_SITL;

    std::vector<uint8_t> msp_message_buffer = std::vector<uint8_t>(sizeof(header));
    std::memcpy(msp_message_buffer.data(), &header, sizeof(header));

    Plugin()->GetEventBus()->Publish<MSPMessageEventArg>("SendMSPMessage", MSPMessageEventArg(MSP_SIMULATOR, msp_message_buffer));
}

void SimData::sendToINAV_HITL()
{
    if (!this->isHitlConnected)
    {
        return;
    }

    auto eventBus = Plugin()->GetEventBus();

    TSimdata simData = this->simDataFromXplane;
    this->applyHardwareFailures(simData);

    TMSPSimulatorToINAV data = {0};

    data.header.version = MSPConstants::MSP_SIMULATOR_VERSION;

    data.header.flags = SIMU_ENABLE |
                 ((this->batEmulation != BATTERY_NONE) ? SIMU_SIMULATE_BATTERY : 0) |
                 (this->muteBeeper ? SIMU_MUTE_BEEPER : 0) |
                 (this->attitude_use_sensors ? SIMU_USE_SENSORS : 0) |
                 (this->GPSHasNewData && !this->gps_timeout ? SIMU_HAS_NEW_GPS_DATA : 0) |
                 (this->batEmulation != BATTERY_NONE ? SIMU_EXT_BATTERY_VOLTAGE : 0) |
                 (this->simulatePitot != TPitotSimulation::None ? SIMU_AIRSPEED : 0) |
                 (this->simulatePitot == TPitotSimulation::Failure ? SIMU2_PITOT_FAILURE : 0) |
                 (this->batEmulation != BATTERY_NONE ? SIMU3_CURRENT_SENSOR : 0) |
                 (this->hasJoystick ? SIMU3_RC_INPUT : 0) |
                 (this->rangefinderSimulation != RANGEFINDER_NONE ? SIMU3_RANGEFINDER : 0) |
                 (this->rxIsFailsafe ? SIMU3_RX_FAILSAFFE : 0);

    this->GPSHasNewData = false;

    data.fix = this->gps_fix;
    data.numSat = (uint8_t)simData.numSats;
    data.lat = (int32_t)round(simData.latitude * 10000000);
    data.lon = (int32_t)round(simData.longitude * 10000000);
    data.alt = (int32_t)round(simData.elevation * 100);      // expected by inav: elevation in cm
    data.speed = (int16_t)round(simData.speed * 100);        // expected by inav: ground speed cm/sec
    data.airspeed = (uint16_t)round(simData.airspeed * 100); // expected by inav: ground speed cm/sec

    data.course = (int16_t)round(simData.course * 10); // expected by inav: deg * 10
    if (data.course < 0)
        data.course += 3600;

    // INAV don't like the velocities from X-Plane and NAV modes go nuts, so send zeroes and let INAV calculate velocities by itself
    data.velNED[0] = 0; // (int16_t)round(-simData.velNED[X] * 100); // nedVelNorth;
    data.velNED[1] = 0; // (int16_t)round(simData.velNED[Y] * 100);  // nedVelEast
    data.velNED[2] = 0; // (int16_t)round(-simData.velNED[Z] * 100); // nedVelDown

    // expected order of rotation from local to global: roll, pitch, yaw
    data.roll = (int16_t)round(simData.euler.roll * 10);   // expected by inav: left wing down - negative roll, 1 degree = 10, values range: -1800...1800
    data.pitch = (int16_t)round(-simData.euler.pitch * 10); // expected by inav: stick down/nose up - negative pitch, upside-down: stick down/nose up - positiive pitch, 1 degree = 10 , values range: -1800...1800
    data.yaw = (int16_t)round(simData.euler.yaw * 10);    // expected by inav: rotate clockwise( top view) - positive yaw+, 1 degreee = 10 , values range: 0...3600 , north = 0
    if (data.yaw < 0)
        data.yaw += 3600;

    
    data.accel_x = Utils::ClampToInt16(-simData.acceleration.x * 1000.0f); // expected by inav: forward - positive
    data.accel_y = Utils::ClampToInt16(simData.acceleration.y * 1000.0f);  // expected by inav: right - negative
    data.accel_z = Utils::ClampToInt16(simData.acceleration.z * 1000.0f);  // expected by inav: 1.0f in stable position (1G)

    data.gyro_x = Utils::ClampToInt16(simData.gyro.x * 16.0f);  // expected by inav: roll left wing down rotation -> negative
    data.gyro_y = Utils::ClampToInt16(-simData.gyro.y * 16.0f); // expected by inav: pitch up rotation -> negative, 1 deerees per second
    data.gyro_z = Utils::ClampToInt16(-simData.gyro.z * 16.0f); // expected by inav: yaw clockwise rotation (top view) ->negative
   
    data.baro = (int32_t)round(simData.baro * 3386.39f);
    data.mag_x = Utils::ClampToInt16(simData.mag.x * 16000.0f);
    data.mag_y = Utils::ClampToInt16(simData.mag.y * 16000.0f);
    data.mag_z = Utils::ClampToInt16(simData.mag.z * 16000.0f);

    data.rangefinder_distance_cm = simData.rangefinder_distance_cm;
    
    this->recalculatePowerTrain();
    data.vbat = (uint8_t)round(this->powerTrain->getCurrentBatteryVoltage() * 10);
    data.current = this->powerTrain->getCurrentBatteryAmps() * 10;

    data.rc_inputs[SimDataConstants::RC_CHANNEL_ROLL] = SimDataConstants::float_minus_1_1_to_pwm(this->rc_inputs[SimDataConstants::RC_CHANNEL_ROLL]);
    data.rc_inputs[SimDataConstants::RC_CHANNEL_PITCH] = SimDataConstants::float_minus_1_1_to_pwm(this->rc_inputs[SimDataConstants::RC_CHANNEL_PITCH]);
    data.rc_inputs[SimDataConstants::RC_CHANNEL_THROTTLE] = SimDataConstants::float_0_1_to_pwm(this->rc_inputs[SimDataConstants::RC_CHANNEL_THROTTLE]);
    data.rc_inputs[SimDataConstants::RC_CHANNEL_YAW] = SimDataConstants::float_minus_1_1_to_pwm(this->rc_inputs[SimDataConstants::RC_CHANNEL_YAW]);
    data.rc_inputs[SimDataConstants::RC_CHANNEL_AUX1] = SimDataConstants::float_0_1_to_pwm(this->rc_inputs[SimDataConstants::RC_CHANNEL_AUX1]);
    data.rc_inputs[SimDataConstants::RC_CHANNEL_AUX2] = SimDataConstants::float_0_1_to_pwm(this->rc_inputs[SimDataConstants::RC_CHANNEL_AUX2]);
    data.rc_inputs[SimDataConstants::RC_CHANNEL_AUX3] = SimDataConstants::float_0_1_to_pwm(this->rc_inputs[SimDataConstants::RC_CHANNEL_AUX3]);
    data.rc_inputs[SimDataConstants::RC_CHANNEL_AUX4] = SimDataConstants::float_0_1_to_pwm(this->rc_inputs[SimDataConstants::RC_CHANNEL_AUX4]);

    // Calculate RSSI based on distance to home location (max range 2 km)
    data.rssi = this->calculateRSSI();

    std::vector<uint8_t> msp_message_buffer = std::vector<uint8_t>(sizeof(data));
    std::memcpy(msp_message_buffer.data(), &data, sizeof(data));

    Plugin()->GetEventBus()->Publish<MSPMessageEventArg>("SendMSPMessage", MSPMessageEventArg(MSP_SIMULATOR, msp_message_buffer));
}


void SimData::updateDataRefs()
{
    TSimdata simData = this->simDataFromXplane;
    this->applyHardwareFailures(simData);

    float throttle = SimDataConstants::input_to_float_0_1(this->control_throttle);
    if (this->batEmulation != BATTERY_NONE)
    {
        throttle = this->powerTrain->getMotorThrottleFactor();
    }
    throttle = std::clamp(throttle, 0.0f, 1.0f);

    UpdateDataRefEventArg eventArgs;
    eventArgs.gpsNumSats = simData.numSats;
    eventArgs.gpsFix = this->gps_fix;
    eventArgs.gpsLatitude = simData.latitude;
    eventArgs.gpsLongitude = simData.longitude;
    eventArgs.gpsElevation = simData.elevation;
    eventArgs.gpsVelocities = simData.velNED;
    eventArgs.groundspeed = simData.speed;
    eventArgs.airspeed = simData.airspeed;
    eventArgs.magnetometer = simData.mag;
    eventArgs.rangefinderDistanceCm = simData.rangefinder_distance_cm;
    eventArgs.batteryVoltage = this->powerTrain->getCurrentBatteryVoltage();
    eventArgs.currentConsumption = this->powerTrain->getCurrentBatteryAmps();
    eventArgs.rssi = this->calculateRSSI();
    eventArgs.isFailsafe = this->rxIsFailsafe;
    
    Plugin()->GetEventBus()->Publish<UpdateDataRefEventArg>("UpdateDataRef", eventArgs);
}
void SimData::applyHardwareFailures(TSimdata &simData)
{
    if (this->simulatePitot == TPitotSimulation::Failure60)
    {
        simData.airspeed = 17.77f; // 60 km/h
    }

    if (this->gps_glitch != SimDataConstants::GPS_GLITCH_NONE)
    {
        if (this->glitch_lattitude == 0)
        {
            this->glitch_lattitude = this->simDataFromXplane.latitude;
            this->glitch_longitude = this->simDataFromXplane.longitude;
            this->glitch_elevation = this->simDataFromXplane.elevation;
        }
    }
    else
    {
        this->glitch_lattitude = 0;
        this->glitch_longitude = 0;
        this->glitch_elevation = 0;
    }

    if (this->gps_glitch == SimDataConstants::GPS_GLITCH_FREEZE)
    {
        simData.latitude = this->glitch_lattitude;
        simData.longitude = this->glitch_longitude;
        simData.elevation = this->glitch_elevation;
        simData.speed = 0;
        simData.velNED.x = 0;
        simData.velNED.y = 0;
        simData.velNED.z = 0;
    }
    else if (this->gps_glitch == SimDataConstants::GPS_GLITCH_OFFSET)
    {
        simData.latitude = this->simDataFromXplane.latitude + 5.0 / 111.32;
        simData.longitude = this->simDataFromXplane.longitude;
        simData.elevation = this->simDataFromXplane.elevation + 50.0f;
    }
    else if (this->gps_glitch == SimDataConstants::GPS_GLITCH_LINEAR)
    {
        float k = Utils::GetTicks() / 100000.0f;
        k -= (int)k;
        simData.latitude = this->simDataFromXplane.latitude + k / 111.32;
        simData.longitude = this->simDataFromXplane.longitude;
        simData.elevation = this->simDataFromXplane.elevation;
    }
    else if (this->gps_glitch == SimDataConstants::GPS_GLITCH_ALTITUDE)
    {
        float k = Utils::GetTicks() / 100000.0f;
        k -= (int)k;
        simData.elevation = this->glitch_elevation + k * 1000.0f;
        simData.velNED.z= k * 1000.0f;
    }

    if (this->simulate_mag_failure)
    {
        simData.mag.x = 0;
        simData.mag.y = 0;
        simData.mag.z = 0;
    }

    if (this->rangefinderSimulation == RANGEFINDER_FAILURE)
    {
        simData.rangefinder_distance_cm = 0;
    }

    if (this->simulatePitot == TPitotSimulation::Failure60)
    {
        simData.airspeed = (uint16_t)round(1777);
    }
}

void SimData::disconnect()
{
    TMSPSimultatorToINAVHeader data;
    data.version = MSPConstants::MSP_SIMULATOR_VERSION;
    data.flags = 0;

    std::vector<uint8_t> msp_message_buffer = std::vector<uint8_t>(sizeof(data));
    std::memcpy(msp_message_buffer.data(), &data, sizeof(data));

    Plugin()->GetEventBus()->Publish<MSPMessageEventArg>("SendMSPMessage", MSPMessageEventArg(MSP_SIMULATOR, msp_message_buffer));

    this->control_throttle = -500;
    this->control_roll = 0;
    this->control_pitch = 0;
    this->control_yaw = 0;

    if (this->isSitlConnected)
    {
        this->sendToXPlane_SITL();
    } 
    else if (this->isHitlConnected)
    {
        this->sendToXPlane_HITL();
    }       
}

void SimData::setBateryEmulation(BatteryEmulationType type)
{
    this->batEmulation = type;
    const BatteryData batteryData = SimDataConstants::BATTERY_DATA.at(type);

    this->powerTrain = std::make_unique<PowerTrain>(batteryData.chemistry, batteryData.capacityMah);
    this->powerTrainLastUpdate = 0.0f;
}

uint16_t SimData::calculateRSSI()
{
    if (!this->homeLocation_isSet || this->rxRangeKm == SimDataConstants::RSSI_INFINITE_RANGE)
    {
        return SimDataConstants::RSSI_MAX_VALUE;
    }

    const double lat_diff = (this->simDataFromXplane.latitude - this->homeLocation_latitude);
    const double lon_diff = (this->simDataFromXplane.longitude - this->homeLocation_longitude) *
                            cos(degreesToRadians(this->homeLocation_latitude));

    const double lat_dist_km = lat_diff * 111.32;
    const double lon_dist_km = lon_diff * 111.32;

    const double alt_diff_km = (this->simDataFromXplane.elevation - this->homeLocation_elevation) / 1000.0;

    double distance_km = sqrt(lat_dist_km * lat_dist_km +
                              lon_dist_km * lon_dist_km +
                              alt_diff_km * alt_diff_km);

    // Inverse proportional RSSI falloff: RSSI = RSSI_MAX / (1 + (distance / reference_distance))
    const double reference_distance = this->rxRangeKm / 2.0;
    const double rssi_value = SimDataConstants::RSSI_MAX_VALUE /
                              (1.0 + (distance_km / reference_distance));

    const uint16_t rssi = std::max(0, std::min(SimDataConstants::RSSI_MAX_VALUE, (int)round(rssi_value)));
    
    
    if (!this->rxIsFailsafeFromMenu)
    {
        this->rxIsFailsafe = rssi < SimDataConstants::RSSI_FAILSAFE_VALUE;
    }
    else
    {
        this->rxIsFailsafe = true;
    }
    return rssi;
}

void SimData::recalculatePowerTrain()
{
    if (this->batEmulation == BATTERY_NONE)
    {
        return;
    }

    if (this->powerTrainLastUpdate == 0.0)
    {
        this->powerTrainLastUpdate = Utils::GetTicks() / 1000.0;
        return;
    }

    const float t = Utils::GetTicks() / 1000.0f; // seconds
    const float dt = t - this->powerTrainLastUpdate;
    this->powerTrainLastUpdate = t;
    this->powerTrain->update(SimDataConstants::input_to_float_0_1(this->control_throttle), this->simDataFromXplane.euler.pitch, dt);
}
