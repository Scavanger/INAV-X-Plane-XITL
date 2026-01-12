#pragma once

#include "platform.h"

#include <functional>

#include "serial/SerialBase.h"

#include "MSP_Commands.h"

namespace MSPConstants
{
    static constexpr int MAX_MSP_MESSAGE = 1024;
    static constexpr int MSP_SIMULATOR_VERSION = 3;
    static constexpr uint8_t XITL_OSD_SIGNATURE = 255;
    static constexpr int OSD_BUFFER_SIZE = 400;
}

typedef enum
{
    FIF_ARMED = 64,
    FIF_IS_AIRPLANE = 128,
    FIF_OSD_DISABLED = 32,
    FIF_ANALOG_OSD_NOT_FOUND = 16
} TFromINAVFlags;

#pragma pack(1)
struct TMSPAPIVersion
{
    uint8_t protocolVersion;
    uint8_t APIMajor;
    uint8_t APIMinor;
};
#pragma pack()

#pragma pack(1)
struct TMSPFCVersion
{
    uint8_t major;
    uint8_t minor;
    uint8_t patchVersion;
};
#pragma pack()

#pragma pack(1)
struct TMSPWPInfo
{
    uint8_t reserved;           // Reserved for waypoint capabilities
    uint8_t maxWaypoints;       // Maximum number of waypoints supported
    uint8_t waypointsListValid; // Is current mission valid
    uint8_t waypointsCount;     // Number of waypoints in current mission
};
#pragma pack()

#pragma pack(1)
struct TMSPWP
{
    uint8_t index;
    uint8_t action;
    int32_t lat;
    int32_t lon;
    int32_t alt; // cm
    uint16_t p1;
    uint16_t p2;
    uint16_t p3;
    uint8_t flags;
};
#pragma pack()

typedef enum
{
    SIMU_ENABLE = (1 << 0),
    SIMU_SIMULATE_BATTERY = (1 << 1),
    SIMU_MUTE_BEEPER = (1 << 2),
    SIMU_USE_SENSORS = (1 << 3),
    SIMU_HAS_NEW_GPS_DATA = (1 << 4),
    SIMU_EXT_BATTERY_VOLTAGE = (1 << 5), // extend MSP_SIMULATOR format 2: battery voltage value
    SIMU_AIRSPEED = (1 << 6),
    SIMU_EXTENDED_FLAGS = (1 << 7), // extend MSP_SIMULATOR format 2: extra flags, not used in V3, but dont delete to keep compatibility

    SIMU2_GPS_TIMEOUT = (1 << 8),
    SIMU2_PITOT_FAILURE = (1 << 9),

    // Format 3
    SIMU3_CURRENT_SENSOR = (1 << 10),
    SIMU3_RC_INPUT = (1 << 11),
    SIMU3_RANGEFINDER = (1 << 12),
    SIMU3_RX_FAILSAFFE = (1 << 13),
    SIMU3_SITL = (1 << 14),
} TSimulatorFlags;

#pragma pack(1)

union TMSPSimulatorOSD
{
    struct
    {
        // new response format
        uint8_t newFormatSignature; // = 255

        // screen size
        uint8_t osdRows; // three high bits - format version
        uint8_t osdCols; // two high bits - reserved

        // starting position of packet
        uint8_t osdRow; // three high bits - reserved
        uint8_t osdCol; // two high bits - reserved

        uint8_t osdRowData[MSPConstants::OSD_BUFFER_SIZE];
    } newFormat;

    struct
    {
        uint8_t osdRow;
        uint8_t osdCol;

        uint8_t osdRowData[MSPConstants::OSD_BUFFER_SIZE];
    } oldFormat;
};

struct TMSPSimulatorFromINAV
{
    int16_t roll;
    int16_t pitch;
    int16_t yaw;
    int16_t throttle;

    uint8_t debugIndex; // | TFromINAVFlags
    int32_t debugValue;

    int16_t estimated_attitude_roll;
    int16_t estimated_attitude_pitch;
    int16_t estimated_attitude_yaw;

    TMSPSimulatorOSD osdData;
};

#pragma pack()

namespace MSPConstants
{
    static constexpr int MSP_SIMULATOR_RESPOSE_MIN_LENGTH = (2 * 4 + 1 + 4 + 1);
}

typedef enum
{
    ConnectedHitl,
    ConnectedSitl,
    ConnectionFailed,
    Disconnected,
    DisconnectedTimeout,
} ConnectionStatus;

class MSP
{
public:
    MSP();
    ~MSP() = default;

    TMSPFCVersion version;

private:
    typedef enum
    {
        STATE_DISCONNECTED,
        STATE_ENUMERATE,
        STATE_ENUMERATE_WAIT,
        STATE_CONNECTED,
        STATE_CONNECT_SERIAL,
        STATE_CONNECT_SERIAL_WAIT,
        STATE_TIMEOUT,
        STATE_CONNECT_TCP,
        STATE_CONNECT_TCP_WAIT
    } TState;

    TState state = STATE_DISCONNECTED;

    typedef enum
    {
        DS_IDLE,
        DS_PROTO_IDENTIFIER,
        DS_DIRECTION_V1,
        DS_DIRECTION_V2,
        DS_FLAG_V2,
        DS_PAYLOAD_LENGTH_V1,
        DS_PAYLOAD_LENGTH_JUMBO_LOW,
        DS_PAYLOAD_LENGTH_JUMBO_HIGH,
        DS_PAYLOAD_LENGTH_V2_LOW,
        DS_PAYLOAD_LENGTH_V2_HIGH,
        DS_CODE_V1,
        DS_CODE_JUMBO_V1,
        DS_CODE_V2_LOW,
        DS_CODE_V2_HIGH,
        DS_PAYLOAD_V1,
        DS_PAYLOAD_V2,
        DS_CHECKSUM_V1,
        DS_CHECKSUM_V2,
    } TDecoderState;

    TDecoderState decoderState = DS_IDLE;

    // Settings
    bool autoDetectPorts = true;
    std::string comPort;
    std::string tcpIp;
    unsigned int tcpPort;

    uint32_t lastUpdate = 0;
    uint32_t reconnectTime = 0;
    bool reconnectToSitl = false;
    bool restartOnAirportLoad = false;

    std::shared_ptr<SerialBase> serial;
    int portID;
    unsigned long probeTime;
#if LIN
    bool probeTtyUSB = false;
#endif

    int unsupported;
    int message_direction;
    int message_length_expected;
    std::vector<uint8_t> message_buffer = std::vector<uint8_t>(MSPConstants::MAX_MSP_MESSAGE);
    int message_length_received;
    int code;
    uint8_t message_checksum;

    void loop();
    void connectDisconnect(bool toSitl);
    void rebootAndReconnect();
    void disconnect();
    bool connectSerialPort(std::string &portName);
    bool connectTCP();
    bool probeNextPort();
    void decode();
    void dispatchMessage(uint8_t expected_checksum);
    bool sendCommand(MSPCommand command);
    bool sendCommand(MSPCommand command, std::vector<uint8_t> &payload);
    void processMessage(std::vector<uint8_t> const &payload);

    uint8_t crc8_dvb_s2(uint8_t crc, unsigned char a) const;
};
