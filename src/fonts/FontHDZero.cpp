#include <vector>

#include <stb_image.h>

#include "../Utils.h"

#include "FontHDZero.h"

static constexpr int OSD_CHAR_WIDTH_24 = 24;
static constexpr int OSD_CHAR_HEIGHT_24 = 36;

static constexpr int OSD_CHAR_WIDTH_36 = 36;
static constexpr int OSD_CHAR_HEIGHT_36 = 54;
static constexpr int CHARS_PER_FONT_ROW = 16;
static constexpr int CHARS_PER_FONT_COLUMN = 32;
static constexpr int CHARS_PER_FILE = (CHARS_PER_FONT_ROW * CHARS_PER_FONT_COLUMN);

FontHDZero::FontHDZero(fs::path path)
{
  this->fontName = path.filename().replace_extension().string();
  unsigned int charByteSize = 0, charByteWidth = 0;
  int width, height, channels;
  uint8_t* image = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
  if (!image)
  {
    throw std::runtime_error("Unable to load font file: " + path.string());
  }

  if ((width != OSD_CHAR_WIDTH_24 * CHARS_PER_FONT_ROW) && (width != OSD_CHAR_WIDTH_36 * CHARS_PER_FONT_ROW)) {
    stbi_image_free(image);
    throw std::runtime_error("Unexpected font size: " + path.string());
  }

  this->charWidth = width / CHARS_PER_FONT_ROW;
  this->charHeight = height / CHARS_PER_FONT_COLUMN;
  charByteSize = this->charWidth * this->charHeight * BYTES_PER_PIXEL_RGBA;
  charByteWidth = this->charWidth * BYTES_PER_PIXEL_RGBA;

  if ((this->charWidth == OSD_CHAR_WIDTH_24) && (this->charHeight != OSD_CHAR_HEIGHT_24)) {
    Utils::LOG("Unexpected image size: {}\n", path.string());
    stbi_image_free(image);
    return;
  }

  if ((this->charWidth == OSD_CHAR_WIDTH_36) && (this->charHeight != OSD_CHAR_HEIGHT_36)) {
    Utils::LOG("Unexpected image size: {}\n", path.string());
    stbi_image_free(image);
    return;
  }

  for (int charIndex = 0; charIndex < CHARS_PER_FILE; charIndex++) {
    std::vector<uint8_t> character(charByteSize);
    int charHeigthIdx = charByteSize - charByteWidth;
    int ix = (charIndex % CHARS_PER_FONT_ROW) * this->charWidth;
    int iy = (charIndex / CHARS_PER_FONT_ROW) * this->charHeight;
    for (unsigned int y = 0; y < this->charHeight; y++) {
      for (unsigned int x = 0; x < this->charWidth; x++) {

        unsigned int idx = ((iy + y) * channels * width) + ((ix + x) * channels);
        uint8_t r = image[idx];
        uint8_t g = image[idx + 1];
        uint8_t b = image[idx + 2];

        if (r != 0x7f || g != 0x7f || b != 0x7f) {
          int cp = charHeigthIdx + (x * BYTES_PER_PIXEL_RGBA);
          character[cp] = r;
          character[cp + 1] = g;
          character[cp + 2] = b;
          character[cp + 3] = 0xff;
        }
      }
      charHeigthIdx -= charByteWidth;
    }
    this->textures.push_back(character);
  }
  stbi_image_free(image);
}

int FontHDZero::getCols()
{
  return 50;
}

int FontHDZero::getRows()
{
  return 18;
}

bool FontHDZero::isAnalog() {
  return false;
}
