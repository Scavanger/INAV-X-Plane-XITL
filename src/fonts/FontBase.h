#pragma once

#include "../platform.h"

#include <cstdint>
#include <vector>
#include <string>

static constexpr int BYTES_PER_PIXEL_RGBA = 4;
static constexpr int MAX_FONT_NAME = MAX_PATH;

class FontBase {

protected:
  std::vector<std::vector<uint8_t>> textures = std::vector<std::vector<uint8_t>>();
  unsigned int charWidth = 0;
  unsigned int charHeight = 0;
  std::string fontName = "";

public:

  unsigned int getCharWidth();
  unsigned int getCharHeight();
  std::string getFontName() { return fontName; }
  std::vector<std::vector<uint8_t>> getTextures();
  virtual int getCols() = 0;
  virtual int getRows() = 0;
  virtual bool isAnalog() = 0;
};