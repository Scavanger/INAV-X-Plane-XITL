#include "Serial.h"

#include "../Utils.h"
#include "TcpSerial.h"

#if LIN
#include <sys/ioctl.h>
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

void TCPSerial::OpenConnection(std::string& connectionString)
{
    if (connectionString.rfind("tcp://", 0) != 0) 
    {
        throw std::invalid_argument("Invalid connection string for TCPSerial. Must start with tcp://");
    }
    
    connectionString = connectionString.substr(6); // Remove "tcp://"
    size_t colonPos = connectionString.find(':');
    if (colonPos == std::string::npos)
    {
        throw std::invalid_argument("Invalid connection string format. Expected format: address:port");
    }

    int port = 0;
    std::string address = connectionString.substr(0, colonPos);
    try 
    {
        port = std::stoi(connectionString.substr(colonPos + 1));
    }
    catch (const std::exception &e)
    {
        throw std::invalid_argument("Invalid port number in connection string.");
    }

#if IBM
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

#if IBM
    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
#else
    this->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif

    if (this->sockfd == INVALID_SOCKET)
    {
#if IBM
        WSACleanup(); // Cleanup on Windows
#endif
        throw std::runtime_error("Failed to create socket");
    }

    struct sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(address.c_str());

    if (connect(this->sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        this->CloseConnection();
        throw std::runtime_error("Failed to connect to the server");
    }
    
#if IBM
    unsigned long one = 1;
    if (ioctlsocket(this->sockfd, FIONBIO, &one) == SOCKET_ERROR) {
#elif LIN
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == SOCKET_ERROR) {
#endif
        this->CloseConnection();
        throw std::runtime_error("Failed to set socket mode to non-blocking");
    }

    this->connected = true;
}

void TCPSerial::CloseConnection()
{
    if (!this->connected) {
        return;
    }

#if IBM
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif
    this->connected = false;
}

std::vector<uint8_t> TCPSerial::ReadData()
{
    if (!this->connected)
    {
        return {};
    }

#if IBM
    unsigned long count = 0;
    int ret = ioctlsocket(this->sockfd, FIONREAD, &count);
#elif LIN
    int count = 0;
    int ret = ioctl(this->sockfd, FIONREAD, &count);
#endif

    if (count <= 0 || ret < 0) {
        return std::vector<uint8_t>();
    }

    std::vector<uint8_t> buffer(count);

#ifdef _WIN32
    int bytesRead = recv(this->sockfd, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0);
#else
    int bytesRead = recv(sockfd, reinterpret_cast<char*>(buffer.data()), buffer.size(), MSG_DONTWAIT);
#endif
    if (bytesRead > 0)
    {
        buffer.resize(bytesRead);
        return buffer;

    }
    else if (bytesRead == 0)
    {
        this->CloseConnection();
        return {};
    }
    else
    {
        // EWOULDBLOCK/EAGAIN
        return {};
    }
}

void TCPSerial::flushOut()
{
    if (!this->connected)
        return;

    if (this->writeBuffer.size() > 0)
    {
#if IBM
        int bytesSent = send(this->sockfd, reinterpret_cast<const char*>(this->writeBuffer.data()), this->writeBuffer.size(), 0);
#else
        int bytesSent = write(this->sockfd, reinterpret_cast<const char*>(this->writeBuffer.data()), this->writeBuffer.size());
#endif

        if (bytesSent == SOCKET_ERROR)
        {
            this->CloseConnection();
        }
    }
    SerialBase::flushOut();
}

TCPSerial::~TCPSerial()
{

    this->CloseConnection();

}
