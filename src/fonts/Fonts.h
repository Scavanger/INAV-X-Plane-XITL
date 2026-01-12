#pragma once

#include "../platform.h"

#include <map>
#include <string>
#include <memory>

#include "FontBase.h"

// PAL screen size
static constexpr int PAL_COLS = 30;
static constexpr int PAL_ROWS = 16;

// NTSC screen size
static constexpr int NTSC_COLS = 30;
static constexpr int NTSC_ROWS = 13;

// HDZERO screen size
static constexpr int HDZERO_COLS = 50;
static constexpr int HDZERO_ROWS = 18;
// Avatar screen size
static constexpr int AVATAR_COLS = 53;
static constexpr int AVATAR_ROWS = 20;

// DJIWTF screen size
static constexpr int DJI_COLS = 60;
static constexpr int DJI_ROWS = 22;

typedef enum
{
    AnalogPAL,
    AnalogNTSC,
    WtfOS,
    HDZero,
    Avatar
} OsdType;

class Fonts
{
private:
    std::map<std::string, std::unique_ptr<FontBase>> analogFonts;
    std::map<std::string, std::unique_ptr<FontBase>> hdZeroFonts;
    std::map<std::string, std::unique_ptr<FontBase>> avatarFonts;
    std::map<std::string, std::unique_ptr<FontBase>> wtfosFonts;
    FontBase* analogFont = nullptr;
    FontBase* hdZeroFont = nullptr;
    FontBase* avatarFont = nullptr;
    FontBase* wtfosFont = nullptr;

public:
    Fonts();

    OsdType osdType;

    void setFontTypeByOsdSize(int rows, int cols);
    void setFontType(OsdType type);
    OsdType getFontTypeByOsdSize(int rows, int cols);
    OsdType getCurrentFontType();
    FontBase* GetCurrentFont();
};