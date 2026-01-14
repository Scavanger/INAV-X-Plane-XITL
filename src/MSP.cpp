#include "MSP.h"
#include "Utils.h"

#include <cstring>

#ifdef APL
#include <iostream>
#include <string>
#include <dirent.h>

#endif

#include "serial/TcpSerial.h"

#include "core/PluginContext.h"
#include "core/EventBus.h"

#include "settings/SettingNames.h"

namespace MSPConstants {
    static constexpr int MSP_DETECT_TIMEOUT_MS = 300;
    static constexpr int MSP_COMM_TIMEOUT_MS = 3000;
    static constexpr int MSP_COMM_DEBUG_TIMEOUT_MS = 60000;
    static constexpr int JUMBO_FRAME_MIN_SIZE = 255;
    static constexpr int RECONNECT_DELAY_MS = 5000;
    static constexpr int MAX_LINUX_TTY_PORTS = 16; // Max /dev/ttyACMx and /dev/ttyUSBx ports to probe, should be enough for everyone
    static constexpr int MAX_WINDOWS_COM_PORTS = 32; // Max COM ports to probe on Windows
    
    // Protocol symbols
    static constexpr char SYM_BEGIN = '$';
    static constexpr char SYM_PROTO_V1 = 'M';
    static constexpr char SYM_PROTO_V2 = 'X';
    static constexpr char SYM_FROM_MWC = '>';
    static constexpr char SYM_TO_MWC = '<';
    static constexpr char SYM_UNSUPPORTED = '!';
}

static constexpr uint32_t MSP_TIMEOUT_MS = 1000u;
static constexpr uint32_t MSP_PERIOD_MS = 10u;

MSP::MSP()
{
    auto eventBus = Plugin()->GetEventBus();

    eventBus->Subscribe<FlightLoopEventArg>("FlightLoop", [this](const FlightLoopEventArg &event)
    { 
        this->loop(); 
    });

    eventBus->Subscribe("AirportLoaded", [this]()
    {
        if (this->restartOnAirportLoad) 
        {
            this->rebootAndReconnect();
        }
    });

    eventBus->Subscribe<MenuConnectEventArg>("MenuConnectDisconnect", [this](const MenuConnectEventArg &event)
    {
        this->connectDisconnect(event.toSitl);
    });

    eventBus->Subscribe<MSPMessageEventArg>("SendMSPMessage", [this](const MSPMessageEventArg &event)
    {
        this->sendCommand(event.command, const_cast<std::vector<uint8_t>&>(event.messageBuffer));
    });

    eventBus->Subscribe<MSPMessageEventArg>("MSPMessage", [this](const MSPMessageEventArg &event)
    {
        if (event.command == MSP_DEBUGMSG)
        {
            Utils::LOG("FC Debug Message: {}", std::string(event.messageBuffer.begin(), event.messageBuffer.end()));
        }
    });

    eventBus->Subscribe<SimulatorConnectedEventArg>("SimulatorConnected", [this](const SimulatorConnectedEventArg &event)
    {
        if (event.status == ConnectionStatus::Disconnected || event.status == ConnectionStatus::DisconnectedTimeout) 
        {
            // Prevent publish event again on disconnect
            this->state = STATE_DISCONNECTED;
            this->disconnect();
        }
    });

    eventBus->Subscribe("MenuRebootINAV", [this]()
    {
        this->rebootAndReconnect();
    });

    eventBus->Subscribe<SettingsChangedEventArg>("SettingsChanged", [this](const SettingsChangedEventArg &event)
    {
        if (event.sectionName == SettingsSections::SECTION_GENERAL) 
        {
            if (event.settingName == SettingsKeys::SETTINGS_AUTODETECT_FC)
            {
               this->autoDetectPorts = event.getValueAs<bool>(true);
            } 
            else if (event.settingName == SettingsKeys::SETTINGS_COM_PORT) 
            {
               this->comPort = event.getValueAs<std::string>("");
            } 
            else if (event.settingName == SettingsKeys::SETTINGS_SITL_IP) 
            {
               this->tcpIp = event.getValueAs<std::string>("127.0.0.1");
            } 
            else if (event.settingName == SettingsKeys::SETTINGS_SITL_PORT) 
            {
               this->tcpPort = event.getValueAs<unsigned int>(5760);
            }
            else if (event.settingName == SettingsKeys::SETTINGS_RESTART_ON_AIRPORT_LOAD)
            {
               this->restartOnAirportLoad = event.getValueAs<bool>(false);
            }
        }
    });

    this->state = STATE_DISCONNECTED;
}

void MSP::connectDisconnect(bool toSitl)
{
    if (this->state != STATE_DISCONNECTED) 
        {
            this->disconnect();
            return;
        } 
        
        if (toSitl) {
            if (!connectTCP())
            {
                Utils::LOG("Failed to connect to SITL at {}:{}", this->tcpIp, this->tcpPort);
                Plugin()->GetEventBus()->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("Failed to connect to SITL", this->tcpIp + ":" + std::to_string(this->tcpPort), 5000));
                this->state = STATE_DISCONNECTED;
            }
        } else {
            if (!this->serial || !this->serial->IsConnected()) {
                if (this->autoDetectPorts) {
                    this->portID = 0; // reset portID for new connection attempt
                    this->state = STATE_ENUMERATE;
#if LIN
                    this->probeTtyUSB = false;
#endif
                    this->probeTime = Utils::GetTicks();
                } else {
#if IBM
                    std::string connectionString = "\\\\.\\" + this->comPort;
#elif LIN
                    std::string connectionString = this->comPort;
#endif
                    
                    if (this->connectSerialPort(connectionString)) {
                        Utils::LOG("Connected to FC on port {}", this->comPort);
                        if (this->sendCommand(MSP_FC_VERSION))
                        {
                            Utils::LOG("MSP_VERSION sent");
                            this->state = STATE_CONNECT_SERIAL_WAIT;
                            this->probeTime = Utils::GetTicks();
                            this->lastUpdate = Utils::GetTicks();
                            this->decoderState = DS_IDLE;
                            return;
                        } 
                        else 
                        {   
                            Utils::LOG("Failed to  send MSP_VERSION command");
                            Plugin()->GetEventBus()->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("Failed send",  "MP_VERSION", 5000));            
                            this->state = STATE_DISCONNECTED;
                        }
                    } 
                    else
                    {
                        Utils::LOG("Failed to connect to FC on port {}", this->comPort);
                        Plugin()->GetEventBus()->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("Failed to connect to", " FC on port " + this->comPort, 5000));
                        this->state = STATE_DISCONNECTED;
                    }
                }
            }
        } 
}

void MSP::rebootAndReconnect()
{
    if (this->state == STATE_DISCONNECTED)
    {
        return;
    }

    this->sendCommand(MSPCommand::MSP_REBOOT);
    this->reconnectTime = Utils::GetTicks() + MSPConstants::RECONNECT_DELAY_MS; // wait 5 seconds before reconnect attempt
    this->reconnectToSitl = typeid(*this->serial.get()) == typeid(TCPSerial);
    this->disconnect();
}

void MSP::dispatchMessage(uint8_t expected_checksum)
{
    if (this->message_checksum == expected_checksum)
    {
        // message received, process
        std::vector<uint8_t> payload(this->message_buffer.begin(), this->message_buffer.begin() + this->message_length_received);
        this->processMessage(payload);
    }
    else
    {
        // console.Utils::LOG('code: ' + this.code + ' - crc failed');
        // this.packet_error++;
        //$('span.packet-error').html(this.packet_error);
    }

    this->decoderState = DS_IDLE;
}

bool MSP::connectSerialPort(std::string &portName)
{
    this->serial = SerialBase::CreateSerial(portName);
    try 
    {
        this->serial->OpenConnection(portName);
    } 
    catch (const std::exception &e) 
    {
        Utils::LOG("Exception while opening serial port {}: {}", portName, e.what());
        this->serial = nullptr;
        return false;
    }
    return this->serial->IsConnected();  
}

bool MSP::probeNextPort()
{
    while (true)
    {
        std::string connectionString;
#if IBM
        portID++; // Start from one since COM0 does not exist
        if (portID == MSPConstants::MAX_WINDOWS_COM_PORTS + 1)
        {
            return false;
        }
        connectionString = "\\\\.\\COM" + std::to_string(portID);
#elif LIN
        if (this->probeTtyUSB)
        {
            if (portID == MSPConstants::MAX_LINUX_TTY_PORTS)
            {
                portID = 0;
                this->probeTtyUSB = false;
                return false;
            }
            connectionString = "/dev/ttyUSB" + std::to_string(portID); 
            
        }
        else
        {
            if (portID == MSPConstants::MAX_LINUX_TTY_PORTS)
            {
                this->probeTtyUSB = true;
                portID = 0;
                continue;
            }
            connectionString = "/dev/ttyACM" + std::to_string(portID);
        }
        portID++;
#endif
        
        Utils::LOG("Probing port {}", connectionString);

        if (!this->connectSerialPort(connectionString))
        {
            continue;
        }
        Utils::LOG("Connected to {}", connectionString);
        if (this->sendCommand(MSP_FC_VERSION))
        {
            Utils::LOG("MSP_FC_VERSION sent");
            this->state = STATE_ENUMERATE_WAIT;
            this->probeTime = Utils::GetTicks();
            this->lastUpdate = Utils::GetTicks();
            this->decoderState = DS_IDLE;
            return true;
        }

        return true;
    }
}

bool MSP::connectTCP()
{
  Utils::LOG("Connecting to {}:{}", this->tcpIp, this->tcpPort);

  std::string connectionString = "tcp://" + this->tcpIp + ":" + std::to_string(this->tcpPort);
  this->serial = SerialBase::CreateSerial(connectionString);
  try {
      this->serial->OpenConnection(connectionString);
  } catch (const std::exception &e) {
      Utils::LOG("Exception while opening TCP connection {}: {}", connectionString, e.what());
      this->serial = nullptr;
      return false;
  }
  
  if (this->serial->IsConnected())
  {
    Utils::LOG("Connected");
    if (this->sendCommand(MSP_FC_VERSION))
    {
      Utils::LOG("MSP_VERSION sent");
      this->state = STATE_CONNECT_TCP_WAIT;

      this->probeTime = Utils::GetTicks();
      this->lastUpdate = Utils::GetTicks();
      this->decoderState = DS_IDLE;
      return true;
    }
  }
  Utils::LOG("Unable to connect");
  return false;
}

void MSP::disconnect()
{
    Utils::LOG("Disconnect");
    if (this->serial)
    {
        this->serial->flushOut();
        if (this->serial->IsConnected())
        {
            Utils::DelayMS(100); // make sure all bytes are sent. 100ms is enought to send 1kb
        }
        this->serial->CloseConnection();
        this->serial = NULL;
    }

    bool timeout = false;
    if (this->state != STATE_DISCONNECTED)
    {
        timeout = this->state == STATE_TIMEOUT;
        this->state = STATE_DISCONNECTED;
        Plugin()->GetEventBus()->Publish<SimulatorConnectedEventArg>("SimulatorConnected", SimulatorConnectedEventArg(timeout ? ConnectionStatus::DisconnectedTimeout : ConnectionStatus::Disconnected));
    }
}

bool MSP::sendCommand(MSPCommand command)
{
    std::vector<uint8_t> emptyPayload;
    return this->sendCommand(command, emptyPayload);
}

bool MSP::sendCommand(MSPCommand command, std::vector<uint8_t> &payload)
{
    if (!this->serial || !this->serial->IsConnected())
    {
        return false;
    }

    int bufferLength = 9;
    int payloadLength = 0;
    if (!payload.empty())
    {
        bufferLength += payload.size();
        payloadLength += payload.size();
    }

    std::vector<uint8_t> buffer = std::vector<uint8_t>(bufferLength);
    buffer[0] = MSPConstants::SYM_BEGIN;
    buffer[1] = MSPConstants::SYM_PROTO_V2;
    buffer[2] = MSPConstants::SYM_TO_MWC;
    buffer[3] = 0;
    buffer[4] = Utils::getLowerByte(static_cast<uint16_t>(command));
    buffer[5] = Utils::getUpperByte(static_cast<uint16_t>(command));
    buffer[6] = Utils::getLowerByte(static_cast<uint16_t>(payloadLength));
    buffer[7] = Utils::getUpperByte(static_cast<uint16_t>(payloadLength));

    if (!payload.empty())
    {
        std::memcpy(&buffer[8], payload.data(), payload.size());
    }

    int crc = 0;
    for (unsigned int i = 3; i < buffer.size() - 1; i++)
    {
        crc = this->crc8_dvb_s2(crc, buffer[i]);
    }
    buffer[buffer.size() - 1] = (uint8_t)crc;

    this->serial->WriteData(buffer);
    
    return true;
}


void MSP::decode()
{
    std::vector<uint8_t> data = this->serial->ReadData();

    if (data.size() > 0)
    {
        this->lastUpdate = Utils::GetTicks();
    }
    else if (Utils::GetTicks() > this->lastUpdate + (Utils::IsDebuggerAttached() ? MSPConstants::MSP_COMM_DEBUG_TIMEOUT_MS : MSPConstants::MSP_COMM_TIMEOUT_MS))
    {
        this->state = STATE_TIMEOUT;
        this->disconnect();
        return;
    }

    for (const uint8_t &c : data)
    {
        switch (this->decoderState)
        {
        case DS_IDLE: // sync char 1
            if (c == MSPConstants::SYM_BEGIN)
            {
                this->decoderState = DS_PROTO_IDENTIFIER;
            }
            break;

        case DS_PROTO_IDENTIFIER: // sync char 2
            switch (c)
            {
            case MSPConstants::SYM_PROTO_V1:
                this->decoderState = DS_DIRECTION_V1;
                break;
            case MSPConstants::SYM_PROTO_V2:
                this->decoderState = DS_DIRECTION_V2;
                break;
            default:
                // unknown protocol
                this->decoderState = DS_IDLE;
            }
            break;

        case DS_DIRECTION_V1: // direction (should be >)

        case DS_DIRECTION_V2:
            this->unsupported = 0;
            switch (c)
            {
            case MSPConstants::SYM_FROM_MWC:
                this->message_direction = 1;
                break;
            case MSPConstants::SYM_TO_MWC:
                this->message_direction = 0;
                break;
            case MSPConstants::SYM_UNSUPPORTED:
                this->unsupported = 1;
                break;
            }
            this->decoderState = this->decoderState == DS_DIRECTION_V1 ? DS_PAYLOAD_LENGTH_V1 : DS_FLAG_V2;
            break;

        case DS_FLAG_V2:
            // Ignored for now
            this->decoderState = DS_CODE_V2_LOW;
            break;
        case DS_PAYLOAD_LENGTH_V1:
            this->message_length_expected = c;

            if (this->message_length_expected == MSPConstants::JUMBO_FRAME_MIN_SIZE)
            {
                this->decoderState = DS_CODE_JUMBO_V1;
            }
            else
            {
                this->message_length_received = 0;
                this->decoderState = DS_CODE_V1;
            }
            break;

        case DS_PAYLOAD_LENGTH_V2_LOW:
            this->message_length_expected = c;
            this->decoderState = DS_PAYLOAD_LENGTH_V2_HIGH;
            break;

        case DS_PAYLOAD_LENGTH_V2_HIGH:
            this->message_length_expected |= c << 8;
            this->message_length_received = 0;
            if (this->message_length_expected <= MSPConstants::MAX_MSP_MESSAGE)
            {
                this->decoderState = this->message_length_expected > 0 ? DS_PAYLOAD_V2 : DS_CHECKSUM_V2;
            }
            else
            {
                // too large payload
                this->decoderState = DS_IDLE;
            }
            break;

        case DS_CODE_V1:
        case DS_CODE_JUMBO_V1:
            this->code = c;
            if (this->message_length_expected > 0)
            {
                // process payload
                if (this->decoderState == DS_CODE_JUMBO_V1)
                {
                    this->decoderState = DS_PAYLOAD_LENGTH_JUMBO_LOW;
                }
                else
                {
                    this->decoderState = DS_PAYLOAD_V1;
                }
            }
            else
            {
                // no payload
                this->decoderState = DS_CHECKSUM_V1;
            }
            break;

        case DS_CODE_V2_LOW:
            this->code = c;
            this->decoderState = DS_CODE_V2_HIGH;
            break;

        case DS_CODE_V2_HIGH:
            this->code |= c << 8;
            this->decoderState = DS_PAYLOAD_LENGTH_V2_LOW;
            break;

        case DS_PAYLOAD_LENGTH_JUMBO_LOW:
            this->message_length_expected = c;
            this->decoderState = DS_PAYLOAD_LENGTH_JUMBO_HIGH;
            break;

        case DS_PAYLOAD_LENGTH_JUMBO_HIGH:
            this->message_length_expected |= c << 8;
            this->message_length_received = 0;
            this->decoderState = DS_PAYLOAD_V1;
            break;

        case DS_PAYLOAD_V1:
        case DS_PAYLOAD_V2:
            this->message_buffer[this->message_length_received] = c;
            this->message_length_received++;

            if (this->message_length_received >= this->message_length_expected)
            {
                this->decoderState = this->decoderState == DS_PAYLOAD_V1 ? DS_CHECKSUM_V1 : DS_CHECKSUM_V2;
            }
            break;

        case DS_CHECKSUM_V1:
            if (this->message_length_expected >= MSPConstants::JUMBO_FRAME_MIN_SIZE)
            {
                this->message_checksum = MSPConstants::JUMBO_FRAME_MIN_SIZE;
            }
            else
            {
                this->message_checksum = this->message_length_expected;
            }
            this->message_checksum ^= this->code;
            if (this->message_length_expected >= MSPConstants::JUMBO_FRAME_MIN_SIZE)
            {
                this->message_checksum ^= this->message_length_expected & 0xFF;
                this->message_checksum ^= (this->message_length_expected & 0xFF00) >> 8;
            }
            for (int ii = 0; ii < this->message_length_received; ii++)
            {
                this->message_checksum ^= this->message_buffer[ii];
            }
            this->dispatchMessage(c);
            break;

        case DS_CHECKSUM_V2:
            this->message_checksum = 0;
            this->message_checksum = this->crc8_dvb_s2(this->message_checksum, 0); // flag
            this->message_checksum = this->crc8_dvb_s2(this->message_checksum, this->code & 0xFF);
            this->message_checksum = this->crc8_dvb_s2(this->message_checksum, (this->code & 0xFF00) >> 8);
            this->message_checksum = this->crc8_dvb_s2(this->message_checksum, this->message_length_expected & 0xFF);
            this->message_checksum = this->crc8_dvb_s2(this->message_checksum, (this->message_length_expected & 0xFF00) >> 8);
            for (int ii = 0; ii < this->message_length_received; ii++)
            {
                this->message_checksum = this->crc8_dvb_s2(this->message_checksum, this->message_buffer[ii]);
            }
            this->dispatchMessage(c);
            break;

        default:
            break;
        }
    }
}

void MSP::processMessage(const std::vector<uint8_t>& payload)
{
    switch (this->state)
    {
    case STATE_ENUMERATE_WAIT:
    case STATE_CONNECT_SERIAL_WAIT:
    case STATE_CONNECT_TCP_WAIT:
    {
        if (this->code != MSP_FC_VERSION)
        {
            break;
        }

        if (payload.size() < sizeof(TMSPFCVersion))
        {
            Utils::LOG("Invalid MSP_FC_VERSION response length: {}", payload.size());
            this->state = STATE_DISCONNECTED;
            break;
        }

        std::memcpy(&this->version, payload.data(), sizeof(TMSPFCVersion));

        Utils::LOG("Connected");
        Utils::LOG("INAV Version {}.{}.{}", this->version.major, this->version.minor, this->version.patchVersion);

        const auto eventArg = SimulatorConnectedEventArg(this->state == STATE_CONNECT_TCP_WAIT ? ConnectionStatus::ConnectedSitl : ConnectionStatus::ConnectedHitl);
        Plugin()->GetEventBus()->Publish<SimulatorConnectedEventArg>("SimulatorConnected", eventArg);
        
        this->state = STATE_CONNECTED;

        break;
    }
    case STATE_CONNECTED:
        Plugin()->GetEventBus()->Publish<MSPMessageEventArg>("MSPMessage", MSPMessageEventArg(static_cast<MSPCommand>(this->code), payload));
        break;
    default:
        break;
    }
}

void MSP::loop()
{
    switch (state)
    {
    case STATE_ENUMERATE:
        if (!this->probeNextPort())
        {
            this->state = STATE_DISCONNECTED;
            Utils::LOG("No FC found on any port");
            Plugin()->GetEventBus()->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("No FC found on", " any port", 5000));
        }
        break;

    case STATE_ENUMERATE_WAIT:
        if (Utils::GetTicks() - this->probeTime > MSPConstants::MSP_DETECT_TIMEOUT_MS)
        {
            Utils::LOG("Probe Timeout");
            this->state = STATE_ENUMERATE;
        }
        else
        {
            this->decode();
        }
        break;
    case STATE_CONNECT_SERIAL_WAIT:
    case STATE_CONNECT_TCP_WAIT:
        if (Utils::GetTicks() - this->probeTime > MSPConstants::MSP_DETECT_TIMEOUT_MS)
        {
            Utils::LOG("Connection Timeout");
            this->disconnect();
            Plugin()->GetEventBus()->Publish<SimulatorConnectedEventArg>("SimulatorConnected", SimulatorConnectedEventArg(ConnectionStatus::ConnectionFailed));
        }
        else
        {
            this->decode();
        }
        break;

    case STATE_CONNECTED:
        this->decode();
        break;
    default:
        break;
    }


    if (this->state == STATE_DISCONNECTED && this->reconnectTime != 0 && Utils::GetTicks() > this->reconnectTime)
    {
        this->connectDisconnect(this->reconnectToSitl); 
        this->reconnectTime = 0;
    }

    if (this->serial)
    {
        this->serial->flushOut();
    }
}

uint8_t MSP::crc8_dvb_s2(uint8_t crc, unsigned char a) const
{
    crc ^= a;
    for (int ii = 0; ii < 8; ++ii)
    {
        if (crc & 0x80)
        {
            crc = (crc << 1) ^ 0xD5;
        }
        else
        {
            crc = crc << 1;
        }
    }
    return crc;
}
