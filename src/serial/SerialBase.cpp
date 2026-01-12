#include "SerialBase.h"

#include "../Utils.h"

#include <memory>

#if LIN
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

#endif

#include "TcpSerial.h"
#include "Serial.h"

const std::shared_ptr<SerialBase> SerialBase::CreateSerial(const std::string &connectionString)
{
    if (connectionString.rfind("tcp://", 0) == 0)
    {
        // TCP Serial
        return std::make_unique<TCPSerial>();
    }
    else
    {
        // Standard Serial
        return std::make_unique<Serial>();
    }
}
SerialBase::SerialBase()
{
    this->connected = false;
    this->writeBuffer.reserve(SERIAL_BUFFER_SIZE);
}

bool SerialBase::WriteData(std::vector<uint8_t>& buffer)
{
    if (!this->IsConnected() || this->writeBuffer.size() + buffer.size() > SERIAL_BUFFER_SIZE)
    {
        return false;
    }

    this->writeBuffer.insert(this->writeBuffer.end(), buffer.begin(), buffer.end());
    
    return true;
}

bool SerialBase::IsConnected()
{
    return this->connected;
}

SerialBase::~SerialBase()
{
    this->CloseConnection();
}

void SerialBase::flushOut()
{
    this->writeBuffer.clear();
}
