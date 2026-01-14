#pragma once

#include "../platform.h"

#include "SerialBase.h"

#if LIN
  typedef int SOCKET;
#endif

#ifndef INVALID_SOCKET
  #define INVALID_SOCKET -1
#endif

class TCPSerial: public SerialBase
{
private:
  SOCKET sockfd = INVALID_SOCKET;

public:
  TCPSerial() = default;
  ~TCPSerial() override;
  void OpenConnection(std::string& connectionString) override;
  void CloseConnection() override;
	std::vector<uint8_t> ReadData() override;
  void flushOut() override;
};
