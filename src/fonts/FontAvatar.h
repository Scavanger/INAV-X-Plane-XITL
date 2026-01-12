#pragma once

#include <filesystem>

#include "FontBase.h"

namespace fs = std::filesystem;

class FontAvatar : public FontBase {
public:
  FontAvatar() = default;
  FontAvatar(fs::path path);
  int getCols();
  int getRows();
  bool isAnalog();
};