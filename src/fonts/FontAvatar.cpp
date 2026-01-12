#include <vector>
#include <algorithm>

#include <stb_image.h>

#include "../Utils.h"

#include "FontAvatar.h"

static constexpr int CHARS_PER_ROW = 2;
static constexpr int CHARS_PER_COLUMN = 256;

struct CharDimensions
{
    unsigned int width;
    unsigned int height;
};

static const std::vector<CharDimensions> avatarCharDimensions = {
    {18, 24},  // 540 p
    {24, 36},  // 720 p
    {36, 54},  // 1080 p
    {48, 72},  // Moonlight 1440 p
    {72, 108}, // Moonlight 2160 p
};

static constexpr int CHARS_PER_FILE = 512;

FontAvatar::FontAvatar(fs::path path)
{
    this->fontName = path.filename().replace_extension().string();

    int width, height, channels;
    unsigned int charByteWidth, charByteSize;

    uint8_t *image = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
    if (!image)
    {
        throw std::runtime_error("Unable to load font file: " + path.string());
    }

    if (channels != BYTES_PER_PIXEL_RGBA)
    {
        stbi_image_free(image);
        throw std::runtime_error("Unexpected image format (not RGBA): " + path.string());
    }

    bool dimensionFound = false;
    for (const auto &dim : avatarCharDimensions)
    {
        // 2 characters per row, 256 rows = 512 characters per file
        if (width == static_cast<int>(dim.width * CHARS_PER_ROW) && height == static_cast<int>(dim.height * CHARS_PER_COLUMN))
        {
            this->charWidth = dim.width;
            this->charHeight = dim.height;
            dimensionFound = true;
            break;
        }
    }

    if (!dimensionFound)
    {
        stbi_image_free(image);
        throw std::runtime_error("Unexpected image size: (" + std::to_string(width) + "x" + std::to_string(height) + ") " + path.string());
    }

    charByteSize = this->charWidth * this->charHeight * BYTES_PER_PIXEL_RGBA;
    charByteWidth = this->charWidth * BYTES_PER_PIXEL_RGBA;

    // Store textures "upside down" for OpenGL (pixel index 0,0 = bottom-left)
    for (int charIndex = 0; charIndex < CHARS_PER_FILE; charIndex++)
    {
        std::vector<uint8_t> character(charByteSize);
        int charHeightIdx = charByteSize - charByteWidth; // Start at bottom

        // Column-major indexing: characters go down rows first, then across columns
        int rowIdx = charIndex % CHARS_PER_COLUMN;
        int colIdx = charIndex / CHARS_PER_COLUMN;

        int imgX = colIdx * this->charWidth;
        int imgY = rowIdx * this->charHeight;

        for (unsigned int y = 0; y < this->charHeight; y++)
        {
            for (unsigned int x = 0; x < this->charWidth; x++)
            {
                // Calculate pixel index in the loaded image
                unsigned int idx = ((imgY + y) * width + (imgX + x)) * BYTES_PER_PIXEL_RGBA;

                uint8_t r = image[idx];
                uint8_t g = image[idx + 1];
                uint8_t b = image[idx + 2];
                uint8_t a = image[idx + 3];

                // Store pixel in character texture (upside down for OpenGL)
                int cp = charHeightIdx + (x * BYTES_PER_PIXEL_RGBA);
                character[cp] = r;
                character[cp + 1] = g;
                character[cp + 2] = b;
                character[cp + 3] = a;
            }
            charHeightIdx -= charByteWidth;
        }

        this->textures.push_back(character);
    }

    stbi_image_free(image);
}

int FontAvatar::getCols()
{
    return 53;
}

int FontAvatar::getRows()
{
    return 20;
}

bool FontAvatar::isAnalog()
{
    return false;
}
