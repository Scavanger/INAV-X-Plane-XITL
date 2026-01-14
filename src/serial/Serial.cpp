#include "../core/PluginContext.h"

#include "Serial.h"

#include "../Utils.h"

#include <cstring>

#if LIN
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

static constexpr int BAUDRATE = 115200;

void Serial::OpenConnection(std::string &connectionString)
{
#if IBM
    this->hSerial = CreateFile(connectionString.c_str(),
                               GENERIC_READ | GENERIC_WRITE,
                               0,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

    if (this->hSerial == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            throw std::runtime_error("Port" + connectionString + " not available");
        }
        else
        {
            throw std::runtime_error("Error connecting to port " + connectionString);
        }
    }
    else
    {
        DCB dcbSerialParams = {0};
        if (!GetCommState(this->hSerial, &dcbSerialParams))
        {
            throw std::runtime_error("Failed to get serial parameters");
        }
        else
        {
            dcbSerialParams.BaudRate = BAUDRATE;
            dcbSerialParams.ByteSize = 8;
            dcbSerialParams.StopBits = ONESTOPBIT;
            dcbSerialParams.Parity = NOPARITY;

            // Disable software flow control (XON/XOFF)
            dcbSerialParams.fOutX = FALSE;
            dcbSerialParams.fInX = FALSE;
            // Disable hardware flow control (RTS/CTS)
            dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
            dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;

            // Disable any special processing of bytes
            dcbSerialParams.fBinary = TRUE;

            if (!SetCommState(hSerial, &dcbSerialParams))
            {
                throw std::runtime_error("Could not set Serial Port parameters");
            }
            else
            {
                COMMTIMEOUTS comTimeOut;
                comTimeOut.ReadIntervalTimeout = MAXDWORD;
                comTimeOut.ReadTotalTimeoutMultiplier = 0;
                comTimeOut.ReadTotalTimeoutConstant = 0;
                comTimeOut.WriteTotalTimeoutMultiplier = 0;
                comTimeOut.WriteTotalTimeoutConstant = 300;
                SetCommTimeouts(hSerial, &comTimeOut);

                this->connected = true;
            }
        }
    }
#elif LIN || APL
    this->fd = open(connectionString.c_str(), O_RDWR);
    if (fd == -1)
    {
        throw std::runtime_error("Couldn't connect to COM port " + connectionString + ": " + std::strerror(errno));
    }

    struct termios terminalOptions = {0};
    tcgetattr(fd, &terminalOptions);

    cfmakeraw(&terminalOptions);

    cfsetispeed(&terminalOptions, BAUDRATE);
    cfsetospeed(&terminalOptions, BAUDRATE);

    terminalOptions.c_cflag = CREAD | CLOCAL;
    terminalOptions.c_cflag |= CS8;
    terminalOptions.c_cflag &= ~HUPCL;

    terminalOptions.c_lflag &= ~ICANON;
    terminalOptions.c_lflag &= ~ECHO;   // Disable echo
    terminalOptions.c_lflag &= ~ECHOE;  // Disable erasure
    terminalOptions.c_lflag &= ~ECHONL; // Disable new-line echo
    terminalOptions.c_lflag &= ~ISIG;   // Disable interpretation of INTR, QUIT and SUSP

    terminalOptions.c_iflag &= ~(IXON | IXOFF | IXANY);                                      // Turn off s/w flow ctrl
    terminalOptions.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

    terminalOptions.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    terminalOptions.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    terminalOptions.c_cc[VMIN] = 0;
    terminalOptions.c_cc[VTIME] = 0;

    int ret = tcsetattr(fd, TCSANOW, &terminalOptions);
    if (ret == -1)
    {
        throw std::runtime_error("Failed to configure device: " + std::string(connectionString) + ": " + std::strerror(errno));
    }

    this->connected = true;
#endif
}

void Serial::CloseConnection()
{

    if (!this->connected) {
        return;
    }

#if IBM
    CloseHandle(this->hSerial);
    
#elif LIN || APL
    close(this->fd);
#endif

this->connected = false;
}

std::vector<uint8_t> Serial::ReadData()
{
    if (!this->connected) {
        return {};
    }

    auto eventBus = Plugin()->GetEventBus();

#if IBM
    COMSTAT status;
    DWORD errors;
    DWORD bytesRead;
    unsigned int toRead;

    ClearCommError(this->hSerial, &errors, &status);
    if (status.cbInQue > 0)
    {
        toRead = (status.cbInQue > SERIAL_BUFFER_SIZE) ? SERIAL_BUFFER_SIZE : status.cbInQue;

        std::vector<uint8_t> buffer(toRead);

        if (ReadFile(this->hSerial, buffer.data(), toRead, &bytesRead, NULL) && bytesRead != 0)
        {
            eventBus->Publish<IntEventArg>("SerialBytesReceived", IntEventArg(static_cast<int>(bytesRead)));
            return buffer;
        }
    }
    return {};
#elif LIN || APL

    int count = 0;
    ioctl(this->fd, FIONREAD, &count);
    if (count <= 0) {
        return {};
    }
    std::vector<uint8_t> buffer(count); 
    int bytesRead = read(this->fd, buffer.data(), buffer.size());
    eventBus->Publish<IntEventArg>("SerialBytesReceived", IntEventArg(static_cast<int>(bytesRead)));
    return buffer;
#endif
}

void Serial::flushOut()
{
    auto plugin = Plugin();
    auto eventBus = plugin->GetEventBus();
    if (this->writeBuffer.size() > 0)
    {
#if IBM
        COMSTAT status;
        DWORD errors;
        DWORD bytesSend;
        if (!WriteFile(this->hSerial, reinterpret_cast<void*>(this->writeBuffer.data()), this->writeBuffer.size(), &bytesSend, 0))
        {
            ClearCommError(this->hSerial, &errors, &status);
            return;
        }

    

#elif LIN || APL
        size_t bytesSend = write(this->fd, this->writeBuffer.data(), this->writeBuffer.size());
#endif
        if (bytesSend != this->writeBuffer.size())
        {
            Utils::LOG("WARN: {} bytes written, but {} bytes requested", bytesSend, this->writeBuffer.size());
        }
        eventBus->Publish<IntEventArg>("SerialBytesSent", IntEventArg(static_cast<int>(bytesSend)));
        
        SerialBase::flushOut();
    }
}

    

Serial::~Serial()
{
    if (this->connected)
    {
        this->CloseConnection();    
    }
}
