#pragma once

#include <filesystem>

#include "FontBase.h"

namespace fs = std::filesystem;
class FontHDZero : public FontBase {
public:
  FontHDZero() = default;
  FontHDZero(fs::path path);
  int getCols();
  int getRows();
  bool isAnalog();
};