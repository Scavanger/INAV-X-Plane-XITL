#pragma once

#include "../platform.h"

#if LIN
#include <sys/types.h>
#include <fcntl.h>
#endif
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

static constexpr int SERIAL_BUFFER_SIZE = 512;

class SerialBase
{
protected:
	bool connected = false;
  std::vector<uint8_t> writeBuffer;

public:
    
  SerialBase();
  static const std::shared_ptr<SerialBase> CreateSerial(const std::string& connectionString);
  
  bool WriteData(std::vector<uint8_t>& buffer);
	bool IsConnected();

  virtual void OpenConnection(std::string& connectionString) = 0;
  virtual void CloseConnection() {};
  virtual ~SerialBase();
  virtual std::vector<uint8_t> ReadData() = 0;
  virtual void flushOut();
  
};
