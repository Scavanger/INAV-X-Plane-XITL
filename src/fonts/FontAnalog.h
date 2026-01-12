#pragma once

#include <filesystem>

namespace fs = std::filesystem;

#include "FontBase.h"


class FontAnalog : public FontBase
{
public:
	FontAnalog() = default;
  FontAnalog(fs::path path);
  int getCols();
  int getRows();
  bool isAnalog();

};
