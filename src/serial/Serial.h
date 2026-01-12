#pragma once

#include "../platform.h"

#include "SerialBase.h"

#ifdef LIN
#include <sys/types.h>
#include <fcntl.h>
#endif



class Serial : public SerialBase
{
private:
#if IBM
  HANDLE hSerial;
#elif LIN || APL
  int fd;
#endif

public:
  Serial() = default;
  ~Serial() override;
  void OpenConnection(std::string& connectionString) override;
  void CloseConnection() override;
	std::vector<uint8_t> ReadData() override;
  void flushOut() override;
};
