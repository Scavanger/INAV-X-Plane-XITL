
#include "FontAnalog.h"
#include "../Utils.h"

#include <stb_image.h>

static constexpr int FONT_IMAGE_WIDTH = 209;
static constexpr int FONT_IMAGE_HEIGHT = 609;

static constexpr int OSD_CHAR_WIDTH = 12;
static constexpr int OSD_CHAR_HEIGHT = 18;  
static constexpr int CHARS_PER_FONT_ROW = 16;
static constexpr int CHARS_PER_FONT_COLUMN = 32;
static constexpr int CHARS_PER_FILE = (CHARS_PER_FONT_ROW * CHARS_PER_FONT_COLUMN);


FontAnalog::FontAnalog(fs::path path) : FontBase()
{
    this->fontName = path.filename().replace_extension().string();

    unsigned int i;
    unsigned int charByteSize = 0, charByteWidth = 0;
    int width, height, channels;
    uint8_t *image = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
    if (!image)
    {
        throw std::runtime_error("Unable to load font file: " + path.string());
    }

    if (width != FONT_IMAGE_WIDTH || height != FONT_IMAGE_HEIGHT)
    {
        throw std::runtime_error("Unexpected image size: " + path.string());
        stbi_image_free(image);

        return;
    }

    this->charWidth = OSD_CHAR_WIDTH;
    this->charHeight = OSD_CHAR_HEIGHT;
    charByteSize = this->charWidth * this->charHeight * BYTES_PER_PIXEL_RGBA;
    charByteWidth = this->charWidth * BYTES_PER_PIXEL_RGBA;

    for (int charIndex = 0; charIndex < CHARS_PER_FILE; charIndex++)
    {
        std::vector<uint8_t> character(charByteSize);
        int charHeigthIdx = charByteSize - charByteWidth;
        int imgXIdx = charIndex % CHARS_PER_FONT_ROW;
        int imgYIdx = charIndex / CHARS_PER_FONT_ROW;
        int ix = imgXIdx * this->charWidth + imgXIdx + 1;  // Border
        int iy = imgYIdx * this->charHeight + imgYIdx + 1; // Border
        for (unsigned int y = 0; y < this->charHeight; y++)
        {
            for (unsigned int x = 0; x < this->charWidth; x++)
            {

                unsigned int idx = ((iy + y) * channels * width) + ((ix + x) * channels);
                uint8_t r = image[idx];
                uint8_t g = image[idx + 1];
                uint8_t b = image[idx + 2];

                if (r != 0x80 || g != 0x80 || b != 0x80)
                {
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

int FontAnalog::getCols()
{
    return 30;
}

int FontAnalog::getRows()
{
    return 16;
}

bool FontAnalog::isAnalog()
{
    return true;
}
