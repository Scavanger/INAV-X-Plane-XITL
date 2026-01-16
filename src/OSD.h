#pragma once

#include "MSP.h"

#include <vector>
#include <memory>

#include "fonts/FontBase.h"
#include "fonts/Fonts.h"
#include "renderer/OsdRenderer.h"

// Toast buffer constants
namespace OSDConstants {
    static constexpr int TOAST_MAX_COLS = 25;
    static constexpr int TOAST_MAX_ROWS = 2;
    
    // Mode constants (bit flags)
    static constexpr uint16_t MAX7456_MODE_BLINK = (1 << 4);
    static constexpr uint16_t CHAR_MODE_EXT = (1 << 2);
    static constexpr uint16_t CHAR_MODE_EXT_DP = 0x100;
    
    // Character mode helper functions
    static inline uint16_t makeCharModeU8(uint8_t c, uint8_t m) {
        return ((((uint16_t)c) << 8) | m);
    }
    
    static inline uint16_t makeCharMode(uint16_t c, uint8_t m) {
        return (makeCharModeU8((uint8_t)c, m) | (c > 255 ? CHAR_MODE_EXT : 0));
    }
    
    static inline uint8_t charByte(uint16_t x) {
        return (x >> 8);
    }
    
    static inline uint8_t modeByte(uint16_t x) {
        return (x & 0xFF);
    }
    
    static inline bool charModeIsExt(uint8_t m) {
        return ((m) & CHAR_MODE_EXT) != 0;
    }
    
    static inline bool charIsBlank(uint16_t x) {
        return ((charByte(x) == 0x20 || charByte(x) == 0x00) && !charModeIsExt(modeByte(x)));
    }
}

typedef enum {
    VIDEO_SYSTEM_AUTO = 0,
    VIDEO_SYSTEM_PAL,
    VIDEO_SYSTEM_NTSC,
    VIDEO_SYSTEM_HDZERO,
    VIDEO_SYSTEM_DJIWTF,
    VIDEO_SYSTEM_AVATAR,
    VIDEO_SYSTEM_DJICOMPAT,
    VIDEO_SYSTEM_DJICOMPAT_HD,
    VIDEO_SYSTEM_DJI_NATIVE
} videoSystem_e;

typedef enum
{
    Auto,
    Linear,
    Nearest
} TOsdFilteringMode;

// OSD dimensions
static constexpr int OSD_MAX_COLS = DJI_COLS;
static constexpr int OSD_MAX_ROWS = DJI_ROWS;

typedef enum
{
    VS_NONE,
    VS_2KM,
    VS_10KM,
    VS_50KM
} TVideoLinkSimulation;

class OSD
{

public:
    OSD();

private:
    bool visible = true;
    TOsdFilteringMode filteringMode = Auto;

    TVideoLinkSimulation videoLink = VS_50KM;

    void updateFromINAV(const TMSPSimulatorOSD& message);
    void updateFromINAVRowData(int osdRow, int osdCol, const uint8_t (&data)[MSPConstants::OSD_BUFFER_SIZE], int decodeRowsCount);

    void makeToast(std::string line1, std::string line2, int durationMs = 3000);

    void disconnect();


    int noiseTextureId = 0;
    int interferenceTextureId = 0;

    int textureWidth = 0;
    int textureHeight = 0;

    double home_lattitude = 0.0f;
    double home_longitude = 0.0f;
    double home_elevation = 0.0f;

    double current_lattitude = 0.0f;
    double current_longitude = 0.0f;
    double current_elevation = 0.0f;

    float roll = 0.0f;

    bool isConnected = false;

    std::unique_ptr<OsdRenderer> osdRenderer = nullptr;
    int noiseTexture = -1;
    int interferenceTexture = -1;

    std::vector<uint16_t> osdData;
    std::vector<uint16_t> toastData;
    uint32_t toastEndTime = 0;

    std::string activeAnalogFont = "";
    std::string activeDigitalFont = "";
    std::vector<FontBase *> fonts;

    void clear();

    void updateFont();
    void drawOSD();
    void drawNoise(float amount);
    void drawInterference(float amount);

    void resetToast();

    float getNoiseAmount();
};
