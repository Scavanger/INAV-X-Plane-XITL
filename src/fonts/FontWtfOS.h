#pragma once

#include <filesystem>
#include <vector>

#include "FontAvatar.h"

// WtfOS is now just pretty much identical to Avatar font, expect for cols/rows
class FontWtfOS : public FontAvatar {
public:

  FontWtfOS(fs::path path) : FontAvatar(path) {}

  int getCols() override { return 60; };
  int getRows() override { return 22; };

};